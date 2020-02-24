/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>
#include <boost/assert.hpp>

#include "common/mp_utils.hpp"
#include "crypto/sr25519_provider.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus,
                            BabeBlockValidator::ValidationError,
                            e) {
  using E = kagome::consensus::BabeBlockValidator::ValidationError;
  switch (e) {
    case E::NO_AUTHORITIES:
      return "no authorities are provided for the validation";
    case E::INVALID_DIGESTS:
      return "the block does not contain valid BABE digests";
    case E::INVALID_SIGNATURE:
      return "SR25519 signature, which is in BABE header, is invalid";
    case E::INVALID_VRF:
      return "VRF value and output are invalid";
    case E::TWO_BLOCKS_IN_SLOT:
      return "peer tried to distribute several blocks in one slot";
    case E::INVALID_TRANSACTIONS:
      return "one or more transactions in the block are invalid";
  }
  return "unknown error";
}

namespace kagome::consensus {
  using common::Buffer;

  BabeBlockValidator::BabeBlockValidator(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::TaggedTransactionQueue> tx_queue,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::SR25519Provider> sr25519_provider)
      : block_tree_{std::move(block_tree)},
        tx_queue_{std::move(tx_queue)},
        hasher_{std::move(hasher)},
        vrf_provider_{std::move(vrf_provider)},
        sr25519_provider_{std::move(sr25519_provider)},
        log_{common::createLogger("BabeBlockValidator")} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(tx_queue_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(sr25519_provider_);
  }

  outcome::result<void> BabeBlockValidator::validate(
      const primitives::Block &block, const Epoch &epoch) const {
    if (epoch.authorities.empty()) {
      return ValidationError::NO_AUTHORITIES;
    }

    log_->debug("Epoch contains authority: {}",
                epoch.authorities.front().id.id.toHex());

    // get BABE-specific digests, which must be inside of this block
    OUTCOME_TRY(babe_digests, getBabeDigests(block));
    auto [seal, babe_header] = babe_digests;

    // signature in seal of the header must be valid
    if (!verifySignature(block, babe_header, seal, epoch.authorities)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (!verifyVRF(babe_header, epoch)) {
      return ValidationError::INVALID_VRF;
    }

    // peer must not send two blocks in one slot
    if (!verifyProducer(babe_header)) {
      return ValidationError::TWO_BLOCKS_IN_SLOT;
    }

    // all transactions in the block must be valid
    if (!verifyTransactions(block.body)) {
      return ValidationError::INVALID_TRANSACTIONS;
    }

    // there must exist a chain with the block in our storage, which is
    // specified as a parent of the block we are validating; BlockTree takes
    // care of this check and returns a specific error, if it fails
    return outcome::success();
  }

  template <typename T, typename VarT>
  boost::optional<T> getFromVariant(VarT &&v) {
    return visit_in_place(
        v,
        [](const T &expected_val) -> boost::optional<T> {
          return boost::get<T>(expected_val);
        },
        [](const auto &) -> boost::optional<T> { return boost::none; });
  }

  outcome::result<std::pair<Seal, BabeBlockHeader>>
  BabeBlockValidator::getBabeDigests(const primitives::Block &block) const {
    // valid BABE block has at least two digests: BabeHeader and a seal
    if (block.header.digest.size() < 2) {
      log_->info(
          "valid BABE block must have at least 2 digests, this one have {}",
          block.header.digest.size());
      return ValidationError::INVALID_DIGESTS;
    }
    const auto &digests = block.header.digest;

    // last digest of the block must be a seal - signature
    auto seal_res = getFromVariant<primitives::Seal>(digests.back());
    if (!seal_res) {
      log_->info("last digest of the block is not a Seal");
      return ValidationError::INVALID_DIGESTS;
    }

    OUTCOME_TRY(babe_seal_res, scale::decode<Seal>(seal_res.value().data));

    for (const auto &digest :
         gsl::make_span(digests).subspan(0, digests.size() - 1)) {
      if (auto consensus_dig = getFromVariant<primitives::PreRuntime>(digest);
          consensus_dig) {
        if (auto header = scale::decode<BabeBlockHeader>(consensus_dig->data);
            header) {
          // found the BabeBlockHeader digest; return
          return {babe_seal_res, header.value()};
        }
      }
    }

    log_->warn("there is no BabeBlockHeader digest in the block");
    return ValidationError::INVALID_DIGESTS;
  }

  bool BabeBlockValidator::verifySignature(
      const primitives::Block &block,
      const BabeBlockHeader &babe_header,
      const Seal &seal,
      gsl::span<const primitives::Authority> authorities) const {
    // firstly, take hash of the block's header without Seal, which is the last
    // digest
    auto block_copy = block;
    block_copy.header.digest.pop_back();

    auto block_copy_encoded = scale::encode(block_copy.header).value();

    auto block_hash = hasher_->blake2b_256(block_copy_encoded);

    // secondly, retrieve public key of the peer by its authority id
    if (static_cast<uint64_t>(authorities.size())
        <= babe_header.authority_index.index) {
      log_->info("don't know about authority with index {}",
                 babe_header.authority_index.index);
      return false;
    }
    const auto &key = authorities[babe_header.authority_index.index].id;

    // thirdly, use verify function to check the signature
    auto res = sr25519_provider_->verify(seal.signature, block_hash, key.id);
    return res && res.value();
  }

  bool BabeBlockValidator::verifyVRF(const BabeBlockHeader &babe_header,
                                     const Epoch &epoch) const {
    // verify VRF output
    auto randomness_with_slot =
        Buffer{}
            .put(epoch.randomness)
            .put(common::uint64_t_to_bytes(babe_header.slot_number));
    auto verify_res = vrf_provider_->verify(
        randomness_with_slot,
        babe_header.vrf_output,
        epoch.authorities[babe_header.authority_index.index].id.id,
        epoch.threshold);
    if (not verify_res.is_valid) {
      log_->error("VRF proof in block is not valid");
      return false;
    }

    // verify threshold
    if (not verify_res.is_less) {
      log_->error("VRF value is not less than the threshold");
      return false;
    }

    return true;
  }

  bool BabeBlockValidator::verifyProducer(
      const BabeBlockHeader &babe_header) const {
    auto peer = babe_header.authority_index;

    auto slot_it = blocks_producers_.find(babe_header.slot_number);
    if (slot_it == blocks_producers_.end()) {
      // this peer is the first producer in this slot
      blocks_producers_.insert({babe_header.slot_number, {peer}});
      return true;
    }

    auto &slot = slot_it->second;
    auto peer_in_slot = slot.find(peer);
    if (peer_in_slot != slot.end()) {
      log_->info("authority {} has already produced a block in the slot {}",
                 peer.index,
                 babe_header.slot_number);
      return false;
    }

    // OK
    slot.insert(peer);
    return true;
  }

  bool BabeBlockValidator::verifyTransactions(
      const primitives::BlockBody &block_body) const {
    return std::all_of(
        block_body.cbegin(), block_body.cend(), [this](const auto &ext) {
          auto validation_res = tx_queue_->validate_transaction(ext);
          if (!validation_res) {
            log_->info("extrinsic validation failed: {}",
                       validation_res.error());
            return false;
          }
          return visit_in_place(
              validation_res.value(),
              [](const primitives::ValidTransaction &) { return true; },
              [](const auto &) { return false; });
        });
  }
}  // namespace kagome::consensus

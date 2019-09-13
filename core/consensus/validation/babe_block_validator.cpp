/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>

#include <boost/assert.hpp>
#include "crypto/sr25519_provider.hpp"
#include "crypto/util.hpp"
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
      std::shared_ptr<crypto::SR25519Provider> sr25519_provider,
      common::Logger log)
      : block_tree_{std::move(block_tree)},
        tx_queue_{std::move(tx_queue)},
        hasher_{std::move(hasher)},
        vrf_provider_{std::move(vrf_provider)},
        sr25519_provider_{std::move(sr25519_provider)},
        log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(tx_queue_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(log_);
  }

  outcome::result<void> BabeBlockValidator::validate(
      const primitives::Block &block, const Epoch &epoch) {
    if (epoch.authorities.empty()) {
      return ValidationError::NO_AUTHORITIES;
    }

    // get BABE-specific digests, which must be inside of this block
    auto digests_res = getBabeDigests(block);
    if (!digests_res) {
      return digests_res.error();
    }
    auto [seal, babe_header] = digests_res.value();

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
    return block_tree_->addBlock(block);
  }

  outcome::result<std::pair<Seal, BabeBlockHeader>>
  BabeBlockValidator::getBabeDigests(const primitives::Block &block) const {
    // valid BABE block has at least two digests: BabeHeader and a seal
    if (block.header.digests.size() < 2) {
      log_->info(
          "valid BABE block must have at least 2 digests, this one have {}",
          block.header.digests.size());
      return ValidationError::INVALID_DIGESTS;
    }
    const auto &digests = block.header.digests;

    // last digest of the block must be a seal - signature
    auto seal_res = scale::decode<Seal>(digests.back());
    if (!seal_res) {
      log_->info("last digest of the block is not a Seal");
      return ValidationError::INVALID_DIGESTS;
    }

    for (const auto &digest :
         gsl::make_span(digests).subspan(0, digests.size() - 1)) {
      if (auto header = scale::decode<BabeBlockHeader>(digest)) {
        // found the BabeBlockHeader digest; return
        return {seal_res.value(), std::move(header.value())};
      }
    }

    log_->info("there is no BabeBlockHeader digest in the block");
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
    block_copy.header.digests.pop_back();

    auto block_copy_encoded_res = scale::encode(block_copy.header);
    if (!block_copy_encoded_res) {
      log_->info("could not encode block header: {}",
                 block_copy_encoded_res.error());
      return false;
    }
    auto block_hash = hasher_->blake2s_256(block_copy_encoded_res.value());

    // secondly, retrieve public key of the peer by its authority id
    if (static_cast<uint64_t>(authorities.size())
        <= babe_header.authority_index) {
      log_->info("don't know about authority with index {}",
                 babe_header.authority_index);
      return false;
    }
    const auto &key = authorities[babe_header.authority_index].id;

    // thirdly, use verify function to check the signature
    auto res = sr25519_provider_->verify(seal.signature, block_hash, key);
    return res && res.value();
  }

  bool BabeBlockValidator::verifyVRF(const BabeBlockHeader &babe_header,
                                     const Epoch &epoch) const {
    // verify VRF output
    auto randomness_with_slot =
        Buffer{}
            .put(epoch.randomness)
            .put(crypto::util::uint256_t_to_bytes(epoch.threshold));
    if (!vrf_provider_->verify(
            randomness_with_slot,
            babe_header.vrf_output,
            epoch.authorities[babe_header.authority_index].id)) {
      log_->info("VRF proof in block is not valid");
      return false;
    }

    // verify threshold
    if (babe_header.vrf_output.value >= epoch.threshold) {
      log_->info("VRF value is not less than the threshold");
      return false;
    }

    return true;
  }

  bool BabeBlockValidator::verifyProducer(const BabeBlockHeader &babe_header) {
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
                 peer,
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
          auto &&[block_number, _] = block_tree_->deepestLeaf();
          auto validation_res =
              tx_queue_->validate_transaction(block_number, ext);
          if (!validation_res) {
            log_->info("extrinsic validation failed: {}",
                       validation_res.error());
            return false;
          }
          return visit_in_place(
              validation_res.value(),
              [](const primitives::Valid &) { return true; },
              [](primitives::Invalid) { return false; },
              [](primitives::Unknown) { return false; });
        });
  }
}  // namespace kagome::consensus

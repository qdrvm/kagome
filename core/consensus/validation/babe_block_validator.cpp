/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>
#include <boost/assert.hpp>

#include "common/mp_utils.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "crypto/sr25519_provider.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus,
                            BabeBlockValidator::ValidationError,
                            e) {
  using E = kagome::consensus::BabeBlockValidator::ValidationError;
  switch (e) {
    case E::NO_AUTHORITIES:
      return "no authorities are provided for the validation";
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

  outcome::result<void> BabeBlockValidator::validateBlock(
      const primitives::Block &block,
      const primitives::AuthorityId &authority_id,
      const Threshold &threshold,
      const Randomness &randomness) const {
    OUTCOME_TRY(
        validateHeader(block.header, authority_id, threshold, randomness));

    // all transactions in the block must be valid
    if (!verifyTransactions(block.body)) {
      return ValidationError::INVALID_TRANSACTIONS;
    }

    // there must exist a chain with the block in our storage, which is
    // specified as a parent of the block we are validating; BlockTree takes
    // care of this check and returns a specific error, if it fails
    return outcome::success();
  }

  outcome::result<void> BabeBlockValidator::validateHeader(
      const primitives::BlockHeader &header,
      const primitives::AuthorityId &authority_id,
      const Threshold &threshold,
      const Randomness &randomness) const {
    log_->debug("Validates block signed by authority: {}",
                authority_id.id.toHex());

    // get BABE-specific digests, which must be inside of this block
    OUTCOME_TRY(babe_digests, getBabeDigests(header));
    auto [seal, babe_header] = babe_digests;

    // signature in seal of the header must be valid
    if (!verifySignature(header, babe_header, seal, authority_id.id)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (!verifyVRF(babe_header, authority_id.id, threshold, randomness)) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

  bool BabeBlockValidator::verifySignature(
      const primitives::BlockHeader &header,
      const BabeBlockHeader &babe_header,
      const Seal &seal,
      const primitives::SessionKey &public_key) const {
    // firstly, take hash of the block's header without Seal, which is the last
    // digest
    auto unsealed_header = header;
    unsealed_header.digest.pop_back();

    auto unsealed_header_encoded = scale::encode(unsealed_header).value();

    auto block_hash = hasher_->blake2b_256(unsealed_header_encoded);

    // secondly, use verify function to check the signature
    auto res =
        sr25519_provider_->verify(seal.signature, block_hash, public_key);
    return res && res.value();
  }

  bool BabeBlockValidator::verifyVRF(const BabeBlockHeader &babe_header,
                                     const primitives::SessionKey &public_key,
                                     const Threshold &threshold,
                                     const Randomness &randomness) const {
    // verify VRF output
    auto randomness_with_slot =
        Buffer{}
            .put(randomness)
            .put(common::uint64_t_to_bytes(babe_header.slot_number));
    auto verify_res = vrf_provider_->verify(
        randomness_with_slot, babe_header.vrf_output, public_key, threshold);
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

  bool BabeBlockValidator::verifyTransactions(
      const primitives::BlockBody &block_body) const {
    return std::all_of(
        block_body.cbegin(), block_body.cend(), [this](const auto &ext) {
          auto validation_res = tx_queue_->validate_transaction(
              primitives::TransactionSource::InBlock, ext);
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

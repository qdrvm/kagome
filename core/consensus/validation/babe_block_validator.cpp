/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>
#include <boost/assert.hpp>

#include "common/mp_utils.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/validation/prepare_transcript.hpp"
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
    case E::SECONDARY_SLOT_ASSIGNMENTS_DISABLED:
      return "Secondary slot assignments are disabled for the current epoch.";
  }
  return "unknown error";
}

namespace kagome::consensus {
  using common::Buffer;
  using primitives::AllowedSlots;

  BabeBlockValidator::BabeBlockValidator(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::TaggedTransactionQueue> tx_queue,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider)
      : block_tree_{std::move(block_tree)},
        tx_queue_{std::move(tx_queue)},
        hasher_{std::move(hasher)},
        vrf_provider_{std::move(vrf_provider)},
        sr25519_provider_{std::move(sr25519_provider)},
        log_{log::createLogger("BlockValidator", "block_validator")} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(tx_queue_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(sr25519_provider_);
  }

  outcome::result<void> BabeBlockValidator::validateHeader(
      const primitives::BlockHeader &header,
      const EpochNumber epoch_number,
      const primitives::AuthorityId &authority_id,
      const Threshold &threshold,
      const primitives::BabeConfiguration &babe_config) const {
    SL_DEBUG(log_, "Validated block signed by authority: {}", authority_id.id);

    // get BABE-specific digests, which must be inside this block
    OUTCOME_TRY(babe_digests, getBabeDigests(header));
    const auto &[seal, babe_header] = babe_digests;

    // @see
    // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/client/consensus/babe/src/verification.rs#L111

    if (babe_header.isProducedInSecondarySlot()) {
      bool plainAndAllowed =
          babe_config.allowed_slots == AllowedSlots::PrimaryAndSecondaryPlain
          and babe_header.slotType() == SlotType::SecondaryPlain;
      bool vrfAndAllowed =
          babe_config.allowed_slots == AllowedSlots::PrimaryAndSecondaryVRF
          and babe_header.slotType() == SlotType::SecondaryVRF;
      if (not plainAndAllowed and not vrfAndAllowed) {
        // SL_WARN unwraps to a lambda which cannot capture a local binding,
        // thus this copy
        auto slot_type = babe_header.slotType();
        SL_WARN(
            log_,
            "Block {} produced in {} slot, but current "
            "configuration allows only {}",
            [&] {
              auto encoded = scale::encode(header).value();
              auto hash = hasher_->blake2b_256(encoded);
              return primitives::BlockInfo(header.number, hash);
            }(),
            to_string(slot_type),
            to_string(babe_config.allowed_slots));
      }
    }

    // signature in seal of the header must be valid
    if (!verifySignature(header,
                         babe_header,
                         seal,
                         primitives::BabeSessionKey{authority_id.id})) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (babe_header.needVRFCheck()
        && !verifyVRF(babe_header,
                      epoch_number,
                      primitives::BabeSessionKey{authority_id.id},
                      threshold,
                      babe_config.randomness,
                      babe_header.needVRFWithThresholdCheck())) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

  bool BabeBlockValidator::verifySignature(
      const primitives::BlockHeader &header,
      const BabeBlockHeader &babe_header,
      const Seal &seal,
      const primitives::BabeSessionKey &public_key) const {
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

  bool BabeBlockValidator::verifyVRF(
      const BabeBlockHeader &babe_header,
      const EpochNumber epoch_number,
      const primitives::BabeSessionKey &public_key,
      const Threshold &threshold,
      const Randomness &randomness,
      const bool checkThreshold) const {
    primitives::Transcript transcript;
    prepareTranscript(
        transcript, randomness, babe_header.slot_number, epoch_number);
    SL_DEBUG(log_,
             "prepareTranscript (verifyVRF): randomness {}, slot {}, epoch {}",
             randomness,
             babe_header.slot_number,
             epoch_number);

    auto verify_res = vrf_provider_->verifyTranscript(
        transcript, babe_header.vrf_output, public_key, threshold);
    if (not verify_res.is_valid) {
      log_->error("VRF proof in block is not valid");
      return false;
    }

    // verify threshold
    if (checkThreshold && not verify_res.is_less) {
      log_->error("VRF value is not less than the threshold");
      return false;
    }

    return true;
  }
}  // namespace kagome::consensus

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_block_validator_impl.hpp"

#include <latch>

#include "consensus/sassafras/impl/sassafras_digests_util.hpp"
#include "consensus/sassafras/sassafras_config_repository.hpp"
#include "consensus/sassafras/sassafras_lottery.hpp"
#include "consensus/sassafras/types/seal.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/vrf_provider.hpp"
#include "metrics/histogram_timer.hpp"
#include "prepare_transcript.hpp"
#include "primitives/inherent_data.hpp"
#include "primitives/transcript.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"
#include "threshold_util.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::sassafras,
                            SassafrasBlockValidatorImpl::ValidationError,
                            e) {
  using E = kagome::consensus::sassafras::SassafrasBlockValidatorImpl::
      ValidationError;
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

namespace kagome::consensus::sassafras {

  SassafrasBlockValidatorImpl::SassafrasBlockValidatorImpl(
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<SassafrasConfigRepository> config_repo,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider)
      : log_(log::createLogger("SassafrasBlockValidatorImpl", "sassafras")),
        slots_util_(std::move(slots_util)),
        config_repo_(std::move(config_repo)),
        hasher_(std::move(hasher)),
        sr25519_provider_(std::move(sr25519_provider)),
        vrf_provider_(std::move(vrf_provider)) {
    BOOST_ASSERT(config_repo_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(vrf_provider_);
  }

  outcome::result<void> SassafrasBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &block_header) const {
    OUTCOME_TRY(slot_claim, sassafras::getSlotClaim(block_header));

    auto slot_number = slot_claim.slot_number;

    OUTCOME_TRY(epoch_number,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               slot_number));

    SL_VERBOSE(
        log_,
        "Appending header of block {} ({} in slot {}, epoch {}, authority #{})",
        block_header.blockInfo(),
        to_string(slot_claim.slotType()),
        slot_number,
        epoch_number,
        slot_claim.authority_index);

    OUTCOME_TRY(config_ptr,
                config_repo_->config(*block_header.parentInfo(), epoch_number));
    auto &config = *config_ptr;

    SL_TRACE(log_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             block_header.blockInfo(),
             slot_number,
             epoch_number,
             config.randomness);

    auto threshold = calculateThreshold(
        config.leadership_rate, config.authorities, slot_claim.authority_index);

    OUTCOME_TRY(
        validateHeader(block_header,
                       epoch_number,
                       config.authorities[slot_claim.authority_index].id,
                       threshold,
                       config));

    return outcome::success();
  }

  outcome::result<void> SassafrasBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &header,
      const EpochNumber epoch_number,
      const AuthorityId &authority_id,
      const Threshold &threshold,
      const SassafrasConfiguration &sassafras_config) const {
    SL_DEBUG(log_, "Validated block signed by authority: {}", authority_id);

    // get BABE-specific digests, which must be inside this block
    OUTCOME_TRY(slot_claim, getSassafrasBlockHeader(header));

    // @see
    // https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/client/consensus/sassafras/src/verification.rs#L111

    if (slot_claim.isProducedInSecondarySlot()) {
      bool plainAndAllowed = sassafras_config.allowed_slots
                              == AllowedSlots::PrimaryAndSecondaryPlain
                         and slot_claim.slotType() == SlotType::SecondaryPlain;
      bool vrfAndAllowed =
          sassafras_config.allowed_slots == AllowedSlots::PrimaryAndSecondaryVRF
          and slot_claim.slotType() == SlotType::SecondaryVRF;
      if (not plainAndAllowed and not vrfAndAllowed) {
        // SL_WARN unwraps to a lambda which cannot capture a local binding,
        // thus this copy
        auto slot_type = slot_claim.slotType();
        SL_WARN(log_,
                "Block {} produced in {} slot, but current "
                "configuration allows only {}",
                header.blockInfo(),
                to_string(slot_type),
                to_string(sassafras_config.allowed_slots));
        return ValidationError::SECONDARY_SLOT_ASSIGNMENTS_DISABLED;
      }
    }

    OUTCOME_TRY(seal, getSeal(header));

    // signature in seal of the header must be valid
    if (!verifySignature(header, seal, authority_id)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (slot_claim.needVRFCheck()
        && !verifyVRF(slot_claim,
                      epoch_number,
                      authority_id,
                      threshold,
                      sassafras_config.randomness,
                      slot_claim.needVRFWithThresholdCheck())) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

  bool SassafrasBlockValidatorImpl::verifySignature(
      const primitives::BlockHeader &header,
      const Seal &seal,
      const AuthorityId &public_key) const {
    primitives::UnsealedBlockHeaderReflection unsealed_header(header);

    auto unsealed_header_encoded = scale::encode(unsealed_header).value();

    auto signed_hash = hasher_->blake2b_256(unsealed_header_encoded);

    // secondly, use verify function to check the signature
    auto res =
        sr25519_provider_->verify(seal.signature, signed_hash, public_key);
    return res && res.value();
  }

  bool SassafrasBlockValidatorImpl::verifyVRF(
      const SassafrasBlockHeader &slot_claim,
      const EpochNumber epoch_number,
      const AuthorityId &public_key,
      const Threshold &threshold,
      const Randomness &randomness,
      const bool checkThreshold) const {
    primitives::Transcript transcript;
    prepareTranscript(
        transcript, randomness, slot_claim.slot_number, epoch_number);
    SL_DEBUG(log_,
             "prepareTranscript (verifyVRF): randomness {}, slot {}, epoch {}",
             randomness,
             slot_claim.slot_number,
             epoch_number);

    auto verify_res = vrf_provider_->verifyTranscript(
        transcript, slot_claim.vrf_output, public_key, threshold);
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
}  // namespace kagome::consensus::sassafras

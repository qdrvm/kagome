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
#include "crypto/bandersnatch_provider.hpp"
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
      std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider)
      : log_(log::createLogger("SassafrasBlockValidatorImpl", "sassafras")),
        slots_util_(std::move(slots_util)),
        config_repo_(std::move(config_repo)),
        hasher_(std::move(hasher)),
        bandersnatch_provider_(std::move(bandersnatch_provider)),
        vrf_provider_(std::move(vrf_provider)) {
    BOOST_ASSERT(config_repo_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(bandersnatch_provider_);
    BOOST_ASSERT(vrf_provider_);
  }

  outcome::result<void> SassafrasBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &block_header) const {
    OUTCOME_TRY(slot_claim, sassafras::getSlotClaim(block_header));

    auto slot_number = slot_claim.slot_number;

    OUTCOME_TRY(epoch_number,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               slot_number));

    SL_VERBOSE(log_,
               "Appending header of block {} ({} claim of slot {}, epoch {}, "
               "authority #{})",
               block_header.blockInfo(),
               slot_claim.ticket_claim ? "primary" : "secondary",
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

    //    auto threshold = calculateThreshold(
    //        config.leadership_rate, config.authorities,
    //        slot_claim.authority_index);

    OUTCOME_TRY(validateHeader(block_header,
                               epoch_number,
                               config.authorities[slot_claim.authority_index],
                               // threshold,
                               config));

    return outcome::success();
  }

  outcome::result<void> SassafrasBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &header,
      const EpochNumber epoch_number,
      const AuthorityId &authority_id,
      // const Threshold &threshold,
      const Epoch &sassafras_config) const {
    SL_DEBUG(log_, "Validated block signed by authority: {}", authority_id);

    OUTCOME_TRY(seal, getSeal(header));

    // signature in seal of the header must be valid
    if (!verifySignature(header, seal, authority_id)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // get Sassafras-specific digests, which must be inside this block
    OUTCOME_TRY(slot_claim, getSlotClaim(header));

    if (slot_claim.ticket_claim.has_value()) {
      SL_VERBOSE(log_,
                 "Block {} produced with primary slot claim (with ticket)",
                 header.blockInfo());

      return verifyPrimaryClaim(slot_claim);
    }

    SL_VERBOSE(log_,
               "Block {} produced with secondary slot claim (without ticket)",
               header.blockInfo());

    return verifySecondaryClaim(slot_claim);
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
        bandersnatch_provider_->verify(seal.signature, signed_hash, public_key);
    return res && res.value();
  }

  outcome::result<void> SassafrasBlockValidatorImpl::verifyPrimaryClaim(
      const SlotClaim &slot_claim) const {
    return outcome::success();  // FIXME
  }

  outcome::result<void> SassafrasBlockValidatorImpl::verifySecondaryClaim(
      const SlotClaim &slot_claim) const {
    return outcome::success();  // FIXME
  }

  //  bool SassafrasBlockValidatorImpl::verifyVRF(
  //      const SassafrasBlockHeader &slot_claim,
  //      const EpochNumber epoch_number,
  //      const AuthorityId &public_key,
  //      const Threshold &threshold,
  //      const Randomness &randomness,
  //      const bool checkThreshold) const {
  //    primitives::Transcript transcript;
  //    prepareTranscript(
  //        transcript, randomness, slot_claim.slot_number, epoch_number);
  //    SL_DEBUG(log_,
  //             "prepareTranscript (verifyVRF): randomness {}, slot {}, epoch
  //             {}", randomness, slot_claim.slot_number, epoch_number);
  //
  //    auto verify_res = vrf_provider_->verifyTranscript(
  //        transcript, slot_claim.vrf_output, public_key, threshold);
  //    if (not verify_res.is_valid) {
  //      log_->error("VRF proof in block is not valid");
  //      return false;
  //    }
  //
  //    // verify threshold
  //    if (checkThreshold && not verify_res.is_less) {
  //      log_->error("VRF value is not less than the threshold");
  //      return false;
  //    }
  //
  //    return true;
  //  }

}  // namespace kagome::consensus::sassafras

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_block_validator_impl.hpp"

#include <latch>

#include "application/app_state_manager.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/sr25519_provider.hpp"
#include "crypto/vrf_provider.hpp"
#include "metrics/histogram_timer.hpp"
#include "prepare_transcript.hpp"
#include "primitives/inherent_data.hpp"
#include "primitives/transcript.hpp"
#include "runtime/runtime_api/babe_api.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "threshold_util.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe,
                            BabeBlockValidatorImpl::ValidationError,
                            e) {
  using E = kagome::consensus::babe::BabeBlockValidatorImpl::ValidationError;
  switch (e) {
    case E::NO_VALIDATOR:
      return "author of block is not active validator";
    case E::DISABLED_VALIDATOR:
      return "author of block is disabled validator";
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

namespace kagome::consensus::babe {

  BabeBlockValidatorImpl::BabeBlockValidatorImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<BabeConfigRepository> config_repo,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider,
      std::shared_ptr<crypto::VRFProvider> vrf_provider,
      std::shared_ptr<runtime::BabeApi> babe_api,
      primitives::events::SyncStateSubscriptionEnginePtr sync_state_observable)
      : log_(log::createLogger("BabeBlockValidatorImpl", "babe")),
        slots_util_(std::move(slots_util)),
        config_repo_(std::move(config_repo)),
        hasher_(std::move(hasher)),
        sr25519_provider_(std::move(sr25519_provider)),
        vrf_provider_(std::move(vrf_provider)),
        babe_api_(std::move(babe_api)),
        sync_state_observable_(std::move(sync_state_observable)) {
    BOOST_ASSERT(config_repo_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(sr25519_provider_);
    BOOST_ASSERT(vrf_provider_);
    BOOST_ASSERT(babe_api_);
    BOOST_ASSERT(sync_state_observable_);

    app_state_manager->takeControl(*this);
  }

  void BabeBlockValidatorImpl::prepare() {
    sync_state_observer_ =
        std::make_shared<primitives::events::SyncStateEventSubscriber>(
            sync_state_observable_, false);
    sync_state_observer_->subscribe(
        sync_state_observer_->generateSubscriptionSetId(),
        primitives::events::SyncStateEventType::kSyncState);
    sync_state_observer_->setCallback(
        [wp{weak_from_this()}](
            auto /*set_id*/,
            bool &synchronized,
            auto /*event_type*/,
            const primitives::events::SyncStateEventParams &event) mutable {
          if (auto self = wp.lock()) {
            if (event == consensus::SyncState::SYNCHRONIZED) {
              self->was_synchronized_ = true;
            }
          }
        });
  }

  outcome::result<void> BabeBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &block_header) const {
    OUTCOME_TRY(babe_header, babe::getBabeBlockHeader(block_header));

    auto slot_number = babe_header.slot_number;

    OUTCOME_TRY(epoch_number,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               slot_number));

    SL_VERBOSE(
        log_,
        "Appending header of block {} ({} in slot {}, epoch {}, authority #{})",
        block_header.blockInfo(),
        to_string(babe_header.slotType()),
        slot_number,
        epoch_number,
        babe_header.authority_index);

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

    if (babe_header.authority_index >= config.authorities.size()) {
      SL_VERBOSE(log_,
                 "Block {} is invalid because validator index out of bound",
                 block_header.blockInfo());
      return ValidationError::NO_VALIDATOR;
    }

    auto threshold = calculateThreshold(config.leadership_rate,
                                        config.authorities,
                                        babe_header.authority_index);

    OUTCOME_TRY(
        validateHeader(block_header,
                       epoch_number,
                       config.authorities[babe_header.authority_index].id,
                       threshold,
                       config));

    // If we were synchronized,
    // we have available runtime to check disabled validators
    if (was_synchronized_) {
      std::vector<AuthorityIndex> disabled_validators;
      if (auto res = babe_api_->disabled_validators(block_header.parent_hash);
          res.has_error()) {
        SL_CRITICAL(log_,
                    "Can't obtain disabled validators list for block {}",
                    block_header.blockInfo());
      }

      if (std::ranges::binary_search(disabled_validators,
                                     babe_header.authority_index)) {
        SL_VERBOSE(log_,
                   "Block {} is invalid because produced by disabled validator",
                   block_header.blockInfo());
        return ValidationError::DISABLED_VALIDATOR;
      }
    }

    return outcome::success();
  }

  outcome::result<void> BabeBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &header,
      const EpochNumber epoch_number,
      const AuthorityId &authority_id,
      const Threshold &threshold,
      const BabeConfiguration &babe_config) const {
    SL_DEBUG(log_, "Validated block signed by authority: {}", authority_id);

    // get BABE-specific digests, which must be inside this block
    OUTCOME_TRY(babe_header, getBabeBlockHeader(header));

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
        SL_WARN(log_,
                "Block {} produced in {} slot, but current "
                "configuration allows only {}",
                header.blockInfo(),
                to_string(slot_type),
                to_string(babe_config.allowed_slots));
        return ValidationError::SECONDARY_SLOT_ASSIGNMENTS_DISABLED;
      }
    }

    OUTCOME_TRY(seal, getSeal(header));

    // signature in seal of the header must be valid
    if (!verifySignature(header, seal, authority_id)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (babe_header.needVRFCheck()
        && !verifyVRF(babe_header,
                      epoch_number,
                      authority_id,
                      threshold,
                      babe_config.randomness,
                      babe_header.needVRFWithThresholdCheck())) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

  bool BabeBlockValidatorImpl::verifySignature(
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

  bool BabeBlockValidatorImpl::verifyVRF(const BabeBlockHeader &babe_header,
                                         const EpochNumber epoch_number,
                                         const AuthorityId &public_key,
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
}  // namespace kagome::consensus::babe

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/sassafras/impl/sassafras_block_validator_impl.hpp"

#include <latch>

#include "consensus/sassafras/impl/sassafras_digests_util.hpp"
#include "consensus/sassafras/impl/sassafras_vrf.hpp"
#include "consensus/sassafras/sassafras_config_repository.hpp"
#include "consensus/sassafras/types/seal.hpp"
#include "consensus/timeline/impl/slot_leadership_error.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "crypto/bandersnatch_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "metrics/histogram_timer.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/runtime_api/sassafras_api.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

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
    case E::WRONG_AUTHOR_OF_SECONDARY_CLAIM:
      return "Wrong author of secondary claim of slot";
    case E::TICKET_UNAVAILABLE:
      return "can't obtain ticket for slot";
    case E::WRONG_PRIMARY_CLAIMING:
      return "Unexpected primary authoring mechanism";
  }
  return "unknown error";
}

namespace kagome::consensus::sassafras {

  SassafrasBlockValidatorImpl::SassafrasBlockValidatorImpl(
      LazySPtr<SlotsUtil> slots_util,
      std::shared_ptr<SassafrasConfigRepository> config_repo,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
      std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
      std::shared_ptr<runtime::SassafrasApi> api)
      : log_(log::createLogger("SassafrasBlockValidator", "sassafras")),
        slots_util_(std::move(slots_util)),
        config_repo_(std::move(config_repo)),
        hasher_(std::move(hasher)),
        bandersnatch_provider_(std::move(bandersnatch_provider)),
        ed25519_provider_(std::move(ed25519_provider)),
        api_(std::move(api)) {
    BOOST_ASSERT(config_repo_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(bandersnatch_provider_);
    BOOST_ASSERT(ed25519_provider_);
    BOOST_ASSERT(api_);
  }

  outcome::result<void> SassafrasBlockValidatorImpl::validateHeader(
      const primitives::BlockHeader &header) const {
    SL_TRACE(log_, "Validating header of block {}...", header.blockInfo());

    // get Sassafras-specific digests, which must be inside this block
    OUTCOME_TRY(slot_claim, getSlotClaim(header));
    OUTCOME_TRY(seal, getSeal(header));

    auto slot = slot_claim.slot_number;

    OUTCOME_TRY(epoch,
                slots_util_.get()->slotToEpoch(*header.parentInfo(), slot));

    OUTCOME_TRY(config_ptr, config_repo_->config(*header.parentInfo(), epoch));
    auto &config = *config_ptr;

    SL_VERBOSE(log_,
               "Validating header of block {}: "
               "{} claim of slot {}, epoch {}, authority #{}",
               header.blockInfo(),
               slot_claim.ticket_claim ? "primary" : "secondary",
               slot,
               epoch,
               slot_claim.authority_index);

    SL_TRACE(log_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             header.blockInfo(),
             slot,
             epoch,
             config.randomness);

    // signature in seal of the header must be valid
    if (!verifySignature(header,
                         seal.signature,
                         config.authorities[slot_claim.authority_index])) {
      return ValidationError::INVALID_SIGNATURE;
    }

    if (slot_claim.ticket_claim.has_value()) {
      OUTCOME_TRY(verifyPrimaryClaim(slot_claim, config, header));
    } else {
      OUTCOME_TRY(verifySecondaryClaim(slot_claim, config));
    }

    return outcome::success();
  }

  bool SassafrasBlockValidatorImpl::verifySignature(
      const primitives::BlockHeader &header,
      const crypto::BandersnatchSignature &signature,
      const Authority &public_key) const {
    primitives::UnsealedBlockHeaderReflection unsealed_header(header);

    auto unsealed_header_encoded = scale::encode(unsealed_header).value();

    auto signed_hash = hasher_->blake2b_256(unsealed_header_encoded);

    // secondly, use verify function to check the signature
    auto res =
        bandersnatch_provider_->verify(signature, signed_hash, public_key);
    return res && res.value();
  }

  outcome::result<void> SassafrasBlockValidatorImpl::verifyPrimaryClaim(
      const SlotClaim &claim,
      const Epoch &config,
      const primitives::BlockHeader &header) const {
    BOOST_ASSERT(claim.ticket_claim.has_value());
    const auto &ticket_claim = claim.ticket_claim.value();

    // Get ticket assigned with slot
    auto ticket_res = api_->slot_ticket(header.parent_hash, claim.slot_number);
    if (ticket_res.has_error()) {
      BOOST_ASSERT_MSG(
          header.number > 0,
          "Blocks of beginning epoch are produced only by secondary way");
      SL_WARN(log_,
              "Can't get ticket for a slot {} on block {}: {}",
              claim.slot_number,
              header.parentInfo().value(),
              ticket_res.error());
      return ValidationError::TICKET_UNAVAILABLE;
    }
    const auto &ticket_opt = ticket_res.value();

    // No ticket expected for ticket claiming
    if (not ticket_opt.has_value()) {
      SL_WARN(log_, "Unexpected primary authoring mechanism");
      return ValidationError::WRONG_PRIMARY_CLAIMING;
    }
    const auto &[ticket_id, ticket_body] = ticket_opt.value();

    // Revealed key check

    auto revealed_key_vrf_input = revealed_key_input(
        config.randomness, ticket_body.attempt_index, config.epoch_index);

    if (claim.signature.outputs.empty()) {
      return ValidationError::INVALID_VRF;
    }
    auto revealed_key_vrf_output = claim.signature.outputs[1];

    auto tmp =
        make_revealed_key_seed(revealed_key_vrf_input, revealed_key_vrf_output);
    EphemeralSeed revealed_seed(
        EphemeralSeed::from(crypto::SecureCleanGuard(tmp)));
    auto revealed_pair =
        ed25519_provider_->generateKeypair(revealed_seed, {}).value();
    if (ticket_body.revealed_public != revealed_pair.secret_key) {
      return ValidationError::INVALID_VRF;
    }

    auto encoded_ticket_body = scale::encode(ticket_body).value();
    std::vector<vrf::BytesIn> transcript_data{encoded_ticket_body};

    auto slot_claim_vrf_input = slot_claim_input(
        config.randomness, claim.slot_number, config.epoch_index);
    std::vector<vrf::VrfInput> inputs;
    inputs.emplace_back(std::move(slot_claim_vrf_input));
    inputs.emplace_back(std::move(revealed_key_vrf_input));

    auto vrf_sign_data =  // consider to call slot_claim_sign_data()
        vrf::vrf_sign_data("sassafras-slot-claim-transcript-v1.0"_bytes,
                           transcript_data,
                           inputs);

    // Optional check, increases some score...
    auto challenge = vrf::vrf_sign_data_challenge<32>(vrf_sign_data);

    if (not ed25519_provider_->verify(ticket_claim.erased_signature,
                                      challenge,
                                      ticket_body.erased_public)) {
      return ValidationError::INVALID_VRF;
    }

    const auto &public_key = config.authorities[claim.authority_index];

    if (not vrf::vrf_verify(claim.signature, vrf_sign_data, public_key)) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

  outcome::result<void> SassafrasBlockValidatorImpl::verifySecondaryClaim(
      const SlotClaim &claim, const Epoch &config) const {
    auto auth_index_of_leader =
        le_bytes_to_uint64(hasher_->blake2b_64(
            scale::encode(config.randomness, claim.slot_number).value()))
        % config.authorities.size();

    if (claim.authority_index != auth_index_of_leader) {
      return ValidationError::WRONG_AUTHOR_OF_SECONDARY_CLAIM;
    }

    auto slot_claim_vrf_input = slot_claim_input(
        config.randomness, claim.slot_number, config.epoch_index);
    std::vector<vrf::VrfInput> inputs;
    inputs.emplace_back(std::move(slot_claim_vrf_input));

    auto vrf_sign_data =  // consider to call slot_claim_sign_data()
        vrf::vrf_sign_data(
            "sassafras-slot-claim-transcript-v1.0"_bytes, {}, inputs);

    //    auto vrf_sign_data = slot_claim_sign_data(
    //        config.randomness, claim.slot_number, config.epoch_index);

    const auto &public_key = config.authorities[claim.authority_index];

    if (not vrf::vrf_verify(claim.signature, vrf_sign_data, public_key)) {
      return ValidationError::INVALID_VRF;
    }

    return outcome::success();
  }

}  // namespace kagome::consensus::sassafras

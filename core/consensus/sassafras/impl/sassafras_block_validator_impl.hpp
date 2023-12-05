/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/sassafras_block_validator.hpp"

#include "clock/clock.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/slot_leadership.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/block.hpp"
#include "primitives/event_types.hpp"
#include "telemetry/service.hpp"

namespace kagome::consensus {
  class SlotsUtil;
}

namespace kagome::consensus::sassafras {
  class SassafrasConfigRepository;
  struct SlotClaim;
  struct Seal;
}  // namespace kagome::consensus::sassafras

namespace kagome::crypto {
  class Hasher;
  class BandersnatchProvider;
  class Ed25519Provider;
  class VRFProvider;
}  // namespace kagome::crypto

namespace kagome::consensus::sassafras {

  class SassafrasBlockValidatorImpl
      : public SassafrasBlockValidator,
        public std::enable_shared_from_this<SassafrasBlockValidatorImpl> {
   public:
    SassafrasBlockValidatorImpl(
        LazySPtr<SlotsUtil> slots_util,
        std::shared_ptr<SassafrasConfigRepository> config_repo,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
        std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<crypto::VRFProvider> vrf_provider);

    outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header) const;

    enum class ValidationError {
      NO_AUTHORITIES = 1,
      INVALID_SIGNATURE,
      INVALID_VRF,
      TWO_BLOCKS_IN_SLOT,
      WRONG_AUTHOR_OF_SECONDARY_CLAIM
    };

   private:
    /**
     * Validate the block header
     * @param block to be validated
     * @param authority_id authority that sent this block
     * @param threshold is vrf threshold for this epoch
     * @param config is sassafras config for this epoch
     * @return nothing or validation error
     */
    outcome::result<void> validateHeader(
        const primitives::BlockHeader &block_header,
        const EpochNumber epoch_number,
        const sassafras::AuthorityId &authority_id,
        // const Threshold &threshold,
        const Epoch &config) const;

    /**
     * Verify that block is signed by valid signature
     * @param header Header to be checked
     * @param seal Seal corresponding to (fetched from) header
     * @param public_key public key that corresponds to the authority by
     * authority index
     * @return true if signature is valid, false otherwise
     */
    bool verifySignature(const primitives::BlockHeader &header,
                         const Signature &signature,
                         const Authority &public_key) const;

    outcome::result<void> verifyPrimaryClaim(const SlotClaim &claim,
                                             const Epoch &config) const;

    outcome::result<void> verifySecondaryClaim(const SlotClaim &claim,
                                               const Epoch &config) const;

    log::Logger log_;

    LazySPtr<SlotsUtil> slots_util_;
    std::shared_ptr<SassafrasConfigRepository> config_repo_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider_;
    std::shared_ptr<crypto::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
  };

}  // namespace kagome::consensus::sassafras

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::sassafras,
                          SassafrasBlockValidatorImpl::ValidationError)

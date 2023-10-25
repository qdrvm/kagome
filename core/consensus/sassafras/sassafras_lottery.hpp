/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus::sassafras {

  class SassafrasLottery {
   public:
    virtual ~SassafrasLottery() = default;

    /**
     * Set new epoch and corresponding randomness, threshold and keypair values
     * @param epoch is an number of epoch where we calculate leadership
     * @param randomness is an epoch random byte sequence
     * @param threshold is a maximum value that is considered valid by vrf
     * @param keypair is a current sassafras sign pair
     */
    virtual void changeEpoch(EpochNumber epoch,
                             const Randomness &randomness,
                             const Threshold &ticket_threshold,
                             const Threshold &threshold,
                             const crypto::Sr25519Keypair &keypair) = 0;

    /**
     * Return lottery current epoch
     */
    virtual EpochNumber getEpoch() const = 0;

    /**
     * Compute leadership for the slot
     * @param slot is a slot number
     * @return none means the peer was not chosen as a leader
     * for that slot, value contains VRF value and proof
     */
    virtual std::optional<crypto::VRFOutput> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const = 0;

    /**
     * Computes VRF proof for the slot regardless threshold.
     * Used when secondary VRF slots are enabled
     * @param slot is a slot number
     * @return VRF output and proof
     */
    virtual crypto::VRFOutput slotVrfSignature(SlotNumber slot) const = 0;

    /**
     * Compute the expected author for secondary slot
     * @param slot - slot to have secondary block produced
     * @param authorities_count - quantity of authorities in current epoch
     * @param randomness - current randomness
     * @return - should always return some authority unless authorities list is
     * empty
     */
    virtual std::optional<primitives::AuthorityIndex> secondarySlotAuthor(
        SlotNumber slot,
        primitives::AuthorityListSize authorities_count,
        const Randomness &randomness) const = 0;
  };

}  // namespace kagome::consensus::sassafras

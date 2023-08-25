/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/timeline/types.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus::babe {
  /**
   * Interface for acquiring leadership information for the current Babe epoch.
   * It is expected to be used as follows:
   *    - first epoch is started, r (randomness) = 0
   *    - blocks are being accepted
   *    - when a block is finalized, submitVRFValue function is called for each
   *      block in the finalized chain up to the last finalized one (in the
   *      chronological order). The chain can be retrieved via calling
   *      BlockTree::longestPath() firstly and only then BlockTree::finalize(..)
   *    - at the end of the epoch, computeRandomness(..) is called, providing a
   *      randomness value for the new epoch
   *    - that value is then is used in slotsLeadership(..)
   *
   */
  class BabeLottery {
   public:
    virtual ~BabeLottery() = default;

    /**
     * Set new epoch and corresponding randomness, threshold and keypair values
     * @param epoch is an information about epoch where we calculate leadership
     * @param randomness is an epoch random byte sequence
     * @param threshold is a maximum value that is considered valid by vrf
     * @param keypair is a current babe sign pair
     */
    virtual void changeEpoch(const EpochDescriptor &epoch,
                             const Randomness &randomness,
                             const Threshold &threshold,
                             const crypto::Sr25519Keypair &keypair) = 0;

    /**
     * Return lottery current epoch
     */
    virtual EpochDescriptor getEpoch() const = 0;

    /**
     * Compute leadership for the slot
     * @param slot is a slot number
     * @return none means the peer was not chosen as a leader
     * for that slot, value contains VRF value and proof
     */
    virtual std::optional<crypto::VRFOutput> getSlotLeadership(
        SlotNumber slot) const = 0;

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
}  // namespace kagome::consensus::babe

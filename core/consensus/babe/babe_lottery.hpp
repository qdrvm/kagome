/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_LOTTERY_HPP
#define KAGOME_BABE_LOTTERY_HPP

#include <boost/optional.hpp>

#include "consensus/babe/types/epoch_descriptor.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus {
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
  struct BabeLottery {
    virtual ~BabeLottery() = default;

    /**
     * Compute leadership for all slots in the given epoch
     * @param epoch is an information about epoch where we calculate leadership
     * @param threshold is a maximum value that is considered valid by vrf
     * @return vector of outputs; none means the peer was not chosen as a leader
     * for that slot, value contains VRF value and proof
     */
    virtual void changeEpoch(const EpochDescriptor &epoch,
                             const Randomness &randomness,
                             const Threshold &threshold,
                             const crypto::Sr25519Keypair &keypair) = 0;

    virtual EpochDescriptor epoch() const = 0;

    virtual boost::optional<crypto::VRFOutput> getSlotLeadership(
        primitives::BabeSlotNumber i) const = 0;

    /**
     * Compute randomness for the next epoch
     * @param last_epoch_randomness - randomness of the last epoch
     * @param new_epoch_number - index of the new epoch
     * @return computed randomness
     *
     * @note must be called exactly ONCE per epoch, when it gets changed
     */
    virtual Randomness computeRandomness(
        const Randomness &last_epoch_randomness,
        EpochNumber new_epoch_number) = 0;

    /**
     * Submit a VRF value for this epoch
     * @param value to be submitted
     *
     * @note concatenation of those values is participating in computation of
     * the randomness for the next epoch
     */
    virtual void submitVRFValue(const crypto::VRFPreOutput &value) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_HPP

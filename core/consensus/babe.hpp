/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_HPP
#define KAGOME_BABE_HPP

#include "clock/clock.hpp"
#include "consensus/babe/types/epoch.hpp"

namespace kagome::consensus {
  /**
   * BABE protocol, used for block production in the Polkadot consensus. One of
   * the two parts in that consensus; the other is Grandpa finality
   * Read more: https://research.web3.foundation/en/latest/polkadot/BABE/Babe/
   */
  struct Babe {
    virtual ~Babe() = default;

    using TimePoint = clock::SystemClock::TimePoint;

    /**
     * Start a Babe production
     * @param epoch - epoch, which is going to be run
     * @param starting_slot_finish_time - when the slot, from which the BABE
     * starts, ends; MUST be less than Clock::now()
     *
     * @note the function will automatically continue launching all further
     * epochs of the Babe production
     * @note in fact, it is an implementation of "Invoke-Block-Authoring" from
     * the spec
     */
    virtual void runEpoch(Epoch epoch, TimePoint starting_slot_finish_time) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_HPP

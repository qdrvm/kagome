/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_HPP
#define KAGOME_BABE_HPP

#include <boost/optional.hpp>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/babe_meta.hpp"
#include "consensus/babe/types/epoch.hpp"
#include "network/babe_observer.hpp"

namespace kagome::consensus {
  /**
   * BABE protocol, used for block production in the Polkadot consensus. One of
   * the two parts in that consensus; the other is Grandpa finality
   * Read more: https://research.web3.foundation/en/latest/polkadot/BABE/Babe/
   */
  class Babe : public network::BabeObserver {
   public:
    ~Babe() override = default;

    enum class ExecutionStrategy {
      GENESIS,
      SYNC_FIRST
    };

    /**
     * Start babe execution
     * @param is_genesis is true when genesis epoch is executed on the current
     * node
     */
    virtual void start(ExecutionStrategy strategy) = 0;

    /**
     * Start a Babe production
     * @param epoch - epoch, which is going to be run
     * @param starting_slot_finish_time - when the slot, from which the BABE
     * starts, ends; for example, we start from
     * 5th slot of the some epoch. Then, we need to set time when 5th slot
     * finishes; most probably, that time will be calculated using Median
     * algorithm
     *
     * @note the function will automatically continue launching all further
     * epochs of the Babe production
     * @note in fact, it is an implementation of "Invoke-Block-Authoring" from
     * the spec
     */
    virtual void runEpoch(Epoch epoch,
                          BabeTimePoint starting_slot_finish_time) = 0;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_HPP

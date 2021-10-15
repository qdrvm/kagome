/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_HPP
#define KAGOME_BABE_HPP

#include <boost/optional.hpp>

#include "consensus/babe/common.hpp"
#include "consensus/babe/types/epoch_descriptor.hpp"
#include "network/block_announce_observer.hpp"

namespace kagome::consensus::babe {
  /**
   * BABE protocol, used for block production in the Polkadot consensus. One of
   * the two parts in that consensus; the other is Grandpa finality
   * Read more: https://research.web3.foundation/en/latest/polkadot/BABE/Babe/
   */
  class Babe : public network::BlockAnnounceObserver {
   public:
    ~Babe() override = default;

    enum class State {
      WAIT_REMOTE_STATUS,  // Node is just executed and waits status of remote
                           // peer to sync missing blocks
      CATCHING_UP,  // Node recognized the missing blocks and started fetching
                    // blocks between the best missing one and one of the
                    // available ones
      WAIT_BLOCK_ANNOUNCE,  // Node is fetched missed blocks and wait block
                            // announce with next block to confirm state
                            // 'synchronized'
      SYNCHRONIZED  // All missing blocks were received and applied, current
                    // peer doing block production
    };

    /**
     * Start a Babe production
     * @param epoch - epoch, which is going to be run
     * epoch.starting_slot_finish_time - when the slot, from which the BABE
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
    virtual void runEpoch(EpochDescriptor epoch) = 0;

    /**
     * @returns current state
     */
    virtual State getCurrentState() const = 0;
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_HPP

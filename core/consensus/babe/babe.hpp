/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/timeline/sync_state.hpp"
#include "consensus/timeline/types.hpp"
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

    /**
     * @returns current state
     */
    virtual SyncState getCurrentState() const = 0;

    /**
     * Checks whether the node was in a synchronized state at least once since
     * startup.
     * @return true when current state was ever set to synchronized during the
     * current run, otherwise - false.
     */
    virtual bool wasSynchronized() const = 0;
  };
}  // namespace kagome::consensus::babe

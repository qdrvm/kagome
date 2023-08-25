/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

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

    enum class State {
      WAIT_REMOTE_STATUS,  // Node is just launched and waits status of remote
                           // peer to sync missing blocks
      HEADERS_LOADING,     // Fast sync requested; phase of headers downloading
      HEADERS_LOADED,      // Fast sync requested; headers downloaded, ready to
                           // syncing of state
      STATE_LOADING,       // Fast sync requested; phase of state downloading
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
     * @returns current state
     */
    virtual State getCurrentState() const = 0;

    /**
     * Checks whether the node was in a synchronized state at least once since
     * startup.
     * @return true when current state was ever set to synchronized during the
     * current run, otherwise - false.
     */
    virtual bool wasSynchronized() const = 0;
  };
}  // namespace kagome::consensus::babe

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace kagome::consensus {

  enum class SyncState : uint8_t {
    /// Node is just launched and waits status of remote peer to sync missing
    /// blocks
    WAIT_REMOTE_STATUS,

    /// Fast sync requested; phase of headers downloading
    HEADERS_LOADING,

    /// Fast sync requested; headers downloaded, ready to syncing of state
    HEADERS_LOADED,

    /// Fast sync requested; phase of state downloading
    STATE_LOADING,

    /// Node recognized the missing blocks and started fetching blocks between
    /// the best missing one and one of the  available ones
    CATCHING_UP,

    /// Node is fetched missed blocks and wait block announce with next block
    /// to confirm state  'synchronized'
    WAIT_BLOCK_ANNOUNCE,

    /// All missing blocks were received and applied, current peer doing
    /// block production
    SYNCHRONIZED
  };

  inline std::string_view to_string(SyncState s) {
    switch (s) {
      case SyncState::WAIT_REMOTE_STATUS:
        return "WAIT_REMOTE_STATUS";
      case SyncState::HEADERS_LOADING:
        return "HEADERS_LOADING";
      case SyncState::HEADERS_LOADED:
        return "HEADERS_LOADED";
      case SyncState::STATE_LOADING:
        return "STATE_LOADING";
      case SyncState::CATCHING_UP:
        return "CATCHING_UP";
      case SyncState::WAIT_BLOCK_ANNOUNCE:
        return "WAIT_BLOCK_ANNOUNCE";
      case SyncState::SYNCHRONIZED:
        return "SYNCHRONIZED";
    }
    __builtin_unreachable();
  }

}  // namespace kagome::consensus

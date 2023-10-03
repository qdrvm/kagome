/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::consensus {

  enum class SyncState {
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

    /// All missing blocks were received and applied, current  peer doing
    /// block production
    SYNCHRONIZED
  };

}

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_SYNCHRONIZER
#define KAGOME_CONSENSUS_BABE_SYNCHRONIZER

#include <libp2p/peer/peer_id.hpp>

#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {

  class BabeSynchronizer {
   public:
    using SyncResultHandler =
        std::function<void(outcome::result<primitives::BlockInfo>)>;

    virtual ~BabeSynchronizer() = default;

    /// Enqueues loading (and applying) blocks from peer {@param peer_id}
    /// since best common block up to provided {@param block_info}.
    /// {@param handler} will be called when this process is finished or failed
    virtual void enqueue(const primitives::BlockInfo &block_info,
                         const libp2p::peer::PeerId &peer_id,
                         SyncResultHandler &&handler) = 0;

    /// Enqueues loading and applying block {@param block_info} from peer
    /// {@param peer_id}.
    /// If provided block is the best after applying, {@param handler} be called
    virtual void enqueue(const primitives::BlockHeader &header,
                         const libp2p::peer::PeerId &peer_id,
                         SyncResultHandler &&handler) = 0;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_BABE_SYNCHRONIZER

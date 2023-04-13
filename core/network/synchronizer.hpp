/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCHRONIZER
#define KAGOME_NETWORK_SYNCHRONIZER

#include <libp2p/peer/peer_id.hpp>

#include "network/types/state_request.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  class Synchronizer {
   public:
    using SyncResultHandler =
        std::function<void(outcome::result<primitives::BlockInfo>)>;
    using CbResultVoid = std::function<void(outcome::result<void>)>;

    virtual ~Synchronizer() = default;

    /// Enqueues loading (and applying) blocks from peer {@param peer_id}
    /// since best common block up to provided {@param block_info}.
    /// {@param handler} will be called when this process is finished or failed
    /// @returns true if sync is ran (peer is not busy)
    /// @note Is used for start/continue catching up.
    virtual bool syncByBlockInfo(const primitives::BlockInfo &block_info,
                                 const libp2p::peer::PeerId &peer_id,
                                 SyncResultHandler &&handler,
                                 bool subscribe_to_block) = 0;

    /// Try to load and apply block with header {@param block_header} from peer
    /// {@param peer_id}.
    /// If provided block is the best after applying, {@param handler} be called
    /// @returns true if sync is ran (peer is not busy)
    /// @note Is used for finish catching up if it possible, and start/continue
    /// than otherwise
    virtual bool syncByBlockHeader(const primitives::BlockHeader &header,
                                   const libp2p::peer::PeerId &peer_id,
                                   SyncResultHandler &&handler) = 0;

    /// Tries to request block justifications from {@param peer_id} for {@param
    /// target_block} or a range of blocks up to {@param limit} count.
    /// Calls {@param handler} when operation finishes
    virtual void syncMissingJustifications(const libp2p::peer::PeerId &peer_id,
                                           primitives::BlockInfo target_block,
                                           std::optional<uint32_t> limit,
                                           SyncResultHandler &&handler) = 0;

    virtual void syncState(const libp2p::peer::PeerId &peer_id,
                           const primitives::BlockInfo &block,
                           SyncResultHandler &&handler) = 0;

    /**
     * Sync block at height 1 for babe genesis slot.
     * Sync block with latest babe consensus digest.
     */
    virtual void syncBabeDigest(const libp2p::peer::PeerId &peer_id,
                                const primitives::BlockInfo &block,
                                CbResultVoid &&cb) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCHRONIZER

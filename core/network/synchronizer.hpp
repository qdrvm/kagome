/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

#include "network/types/state_request.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {
  struct GrandpaJustification;
}  // namespace kagome::consensus::grandpa

namespace kagome::network {
  using consensus::grandpa::GrandpaJustification;
  using libp2p::PeerId;
  using primitives::BlockHeader;
  using primitives::BlockNumber;

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

    /// Fetch justification
    virtual bool fetchJustification(const primitives::BlockInfo &block,
                                    CbResultVoid cb) = 0;

    /// Fetch justification range
    using FetchJustificationRangeCb = std::function<void(
        outcome::result<std::optional<primitives::BlockNumber>>)>;
    virtual bool fetchJustificationRange(primitives::BlockNumber min,
                                         FetchJustificationRangeCb cb) = 0;

    /// Try to launch fetching and storing block headers process.
    /// Fetch of full range is not guaranteed, might be limited by block
    /// response size e.g.
    /// @param max block to start fetching from
    /// @param min block to be fetched last and stop process
    /// @param isFinalized if max is finalized block
    /// @param cb will be called when the launched process is finished or failed
    /// @returns true if fetch is successfully started
    virtual bool fetchHeadersBack(const primitives::BlockInfo &max,
                                  primitives::BlockNumber min,
                                  bool isFinalized,
                                  CbResultVoid cb) = 0;

    virtual void syncState(const libp2p::peer::PeerId &peer_id,
                           const primitives::BlockInfo &block,
                           SyncResultHandler &&handler) = 0;

    using UnsafeOk = std::pair<BlockHeader, GrandpaJustification>;
    using UnsafeRes = std::variant<BlockNumber, UnsafeOk>;
    using UnsafeCb = std::function<void(UnsafeRes)>;
    /// Fetch headers back from `max` until block with justification for grandpa
    /// scheduled change.
    /// @returns justification or next number to use as `max`
    virtual void unsafe(PeerId peer_id, BlockNumber max, UnsafeCb cb) = 0;
  };

}  // namespace kagome::network

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
  using primitives::BlockInfo;
  using primitives::BlockNumber;

  class Synchronizer {
   public:
    using CbResultVoid = std::function<void(outcome::result<void>)>;

    virtual ~Synchronizer() = default;

    /// Block announce handshake received from peer.
    virtual void onBlockAnnounceHandshake(const BlockInfo &block_info,
                                          const PeerId &peer_id) = 0;

    /// Block announce received from peer.
    virtual void onBlockAnnounce(const BlockHeader &header,
                                 const PeerId &peer_id) = 0;

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

    using SyncStateCb = std::function<void()>;
    /// Start or continue state sync for block.
    virtual void syncState(const BlockInfo &block, SyncStateCb handler) = 0;

    virtual void fetchGrandpaFork(const PeerId &peer_id,
                                  const primitives::BlockInfo &vote) = 0;

    using UnsafeOk = std::pair<BlockHeader, GrandpaJustification>;
    using UnsafeRes = std::variant<BlockNumber, UnsafeOk>;
    using UnsafeCb = std::function<void(UnsafeRes)>;
    /// Fetch headers back from `max` until block with justification for grandpa
    /// scheduled change.
    /// @returns justification or next number to use as `max`
    virtual void unsafe(PeerId peer_id, BlockNumber max, UnsafeCb cb) = 0;
  };

}  // namespace kagome::network

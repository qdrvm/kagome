/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/synchronizer.hpp"

#include "consensus/grandpa/structs.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class SynchronizerMock : public Synchronizer {
   public:
    MOCK_METHOD(bool,
                syncByBlockInfo,
                (const primitives::BlockInfo &,
                 const libp2p::peer::PeerId &,
                 const SyncResultHandler &,
                 bool),
                ());
    bool syncByBlockInfo(const primitives::BlockInfo &block_info,
                         const libp2p::peer::PeerId &peer_id,
                         SyncResultHandler &&handler,
                         bool subscribe_to_block) override {
      return syncByBlockInfo(block_info, peer_id, handler, subscribe_to_block);
    }

    MOCK_METHOD(bool,
                syncByBlockHeader,
                (const primitives::BlockHeader &,
                 const libp2p::peer::PeerId &,
                 const SyncResultHandler &),
                ());
    bool syncByBlockHeader(const primitives::BlockHeader &block_header,
                           const libp2p::peer::PeerId &peer_id,
                           SyncResultHandler &&handler) override {
      return syncByBlockHeader(block_header, peer_id, handler);
    }

    MOCK_METHOD(bool,
                fetchJustification,
                (const primitives::BlockInfo &, CbResultVoid),
                (override));

    MOCK_METHOD(bool,
                fetchJustificationRange,
                (primitives::BlockNumber, FetchJustificationRangeCb),
                (override));

    MOCK_METHOD(void,
                syncState,
                (const libp2p::peer::PeerId &,
                 const primitives::BlockInfo &,
                 const SyncResultHandler &),
                ());

    MOCK_METHOD(bool,
                fetchHeadersBack,
                (const primitives::BlockInfo &,
                 primitives::BlockNumber,
                 bool,
                 CbResultVoid),
                (override));

    void syncState(const libp2p::peer::PeerId &peer_id,
                   const primitives::BlockInfo &block_info,
                   SyncResultHandler &&handler) override {
      return syncState(peer_id, block_info, handler);
    }

    MOCK_METHOD(void, unsafe, (PeerId, BlockNumber, UnsafeCb), (override));
  };

}  // namespace kagome::network

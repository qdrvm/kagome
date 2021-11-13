/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCHRONIZERMOCK
#define KAGOME_NETWORK_SYNCHRONIZERMOCK

#include "network/synchronizer.hpp"

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
    };

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
    };
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCHRONIZERMOCK

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP

#include "consensus/babe/babe_synchronizer.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BabeSynchronizerMock : public BabeSynchronizer {
   public:
    MOCK_METHOD3(syncByBlockInfo,
                 void(const primitives::BlockInfo &,
                      const libp2p::peer::PeerId &,
                      const SyncResultHandler &));

    void syncByBlockInfo(const primitives::BlockInfo &block_info,
                         const libp2p::peer::PeerId &peer_id,
                         SyncResultHandler &&handler) override {
      const auto &handler_cref = handler;
      syncByBlockInfo(block_info, peer_id, handler_cref);
    };

    MOCK_METHOD3(syncByBlockHeader,
                 void(const primitives::BlockHeader &,
                      const libp2p::peer::PeerId &,
                      const SyncResultHandler &));

    void syncByBlockHeader(const primitives::BlockHeader &block_header,
                           const libp2p::peer::PeerId &peer_id,
                           SyncResultHandler &&handler) override {
      const auto &handler_cref = handler;
      syncByBlockHeader(block_header, peer_id, handler_cref);
    };
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP

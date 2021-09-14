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
    MOCK_METHOD3(enqueue_rv,
                 void(const primitives::BlockInfo &,
                      const libp2p::peer::PeerId &,
                      const SyncResultHandler &));

    void enqueue(const primitives::BlockInfo &block_info,
                 const libp2p::peer::PeerId &peer_id,
                 SyncResultHandler &&handler) {
      enqueue_rv(block_info, peer_id, handler);
    };
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP

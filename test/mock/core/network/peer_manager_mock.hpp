/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERMANAGERMOCK
#define KAGOME_NETWORK_PEERMANAGERMOCK

#include "network/peer_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class PeerManagerMock final : public PeerManager {
   public:
    MOCK_METHOD1(connectToPeer, void(const PeerInfo &));
    MOCK_CONST_METHOD1(reserveStreams, void(const PeerId &));
    MOCK_METHOD1(keepAlive, void(const PeerId &));
    MOCK_METHOD1(startPingingPeer, void(const PeerId &));

    MOCK_METHOD2(updatePeerStatus, void(const PeerId &, const Status &));
    MOCK_METHOD2(updatePeerStatus, void(const PeerId &, const BlockInfo &));
    MOCK_METHOD1(getPeerStatus, boost::optional<Status>(const PeerId &));
    MOCK_CONST_METHOD0(activePeersNumber, size_t());
    MOCK_CONST_METHOD1(forEachPeer,
                       void(std::function<void(const PeerId &)> func));
    MOCK_CONST_METHOD2(forOnePeer,
                       void(const PeerId &peer_id,
                            std::function<void(const PeerId &)> func));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERMANAGERMOCK

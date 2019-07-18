/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_MOCK_HPP
#define KAGOME_CONNECTION_MANAGER_MOCK_HPP

#include <vector>

#include <gmock/gmock.h>
#include "libp2p/network/connection_manager.hpp"

namespace libp2p::network {

  struct ConnectionManagerMock : public ConnectionManager {
    ~ConnectionManagerMock() override = default;

    MOCK_CONST_METHOD0(getConnections, std::vector<ConnectionSPtr>());
    MOCK_CONST_METHOD1(getConnectionsToPeer,
                       std::vector<ConnectionSPtr>(const peer::PeerId &p));
    MOCK_CONST_METHOD1(getBestConnectionForPeer,
                       ConnectionSPtr(const peer::PeerId &p));
    MOCK_CONST_METHOD1(connectedness, Connectedness(const peer::PeerInfo &p));

    MOCK_METHOD2(addConnectionToPeer,
                 void(const peer::PeerId &p, ConnectionSPtr c));

    MOCK_METHOD1(closeConnectionsToPeer, void(const peer::PeerId &p));

    MOCK_METHOD0(collectGarbage, void());
  };

}  // namespace libp2p::network

#endif  // KAGOME_CONNECTION_MANAGER_MOCK_HPP

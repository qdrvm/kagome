/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_MOCK_HPP
#define KAGOME_NETWORK_MOCK_HPP

#include "libp2p/network/network.hpp"

#include <gmock/gmock.h>

namespace libp2p::network {
  struct NetworkMock : public Network {
    MOCK_CONST_METHOD0(getPeers, std::vector<peer::PeerId>());

    MOCK_CONST_METHOD0(getConnections, std::vector<connptr_t>());

    MOCK_CONST_METHOD1(getConnectionsForPeer,
                       std::vector<connptr_t>(const peer::PeerId &));

    MOCK_CONST_METHOD1(getBestConnectionForPeer,
                       connptr_t(const peer::PeerId &));

    MOCK_CONST_METHOD1(connectedness, Connectedness(const peer::PeerId &));

    MOCK_METHOD2(dial,
                 void(const peer::PeerInfo &,
                      std::function<void(outcome::result<connptr_t>)>));

    MOCK_METHOD1(close, outcome::result<void>(const peer::PeerInfo &));

    MOCK_METHOD1(listen, outcome::result<void>(const multi::Multiaddress &));

    MOCK_CONST_METHOD0(getListenAddresses, std::vector<multi::Multiaddress>());

    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &, const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &));
  };
}  // namespace libp2p::network

#endif  // KAGOME_NETWORK_MOCK_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_MOCK_HPP
#define KAGOME_HOST_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/host.hpp"

namespace libp2p {
  class HostMock : public Host {
   public:
    MOCK_CONST_METHOD0(getLibp2pVersion, std::string_view());

    MOCK_CONST_METHOD0(getLibp2pClientVersion, std::string_view());

    MOCK_CONST_METHOD0(getId, peer::PeerId());

    MOCK_CONST_METHOD0(getPeerInfo, peer::PeerInfo());

    MOCK_CONST_METHOD0(getListenAddresses,
                       gsl::span<const multi::Multiaddress>());

    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &));

    MOCK_METHOD3(setProtocolHandler,
                 void(const std::string &,
                      const std::function<connection::Stream::Handler> &,
                      const std::function<bool(const peer::Protocol &)> &));

    MOCK_METHOD1(connect, outcome::result<void>(const peer::PeerInfo &));

    MOCK_METHOD3(newStream,
                 outcome::result<void>(
                     const peer::PeerInfo &, const peer::Protocol &,
                     const std::function<connection::Stream::Handler> &));

    MOCK_CONST_METHOD0(network, const network::Network &());

    MOCK_CONST_METHOD0(peerRepository, peer::PeerRepository &());

    MOCK_CONST_METHOD0(router, const network::Router &());
  };
}  // namespace libp2p

#endif  // KAGOME_HOST_MOCK_HPP

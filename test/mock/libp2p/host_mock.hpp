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

    MOCK_CONST_METHOD0(getAddresses, gsl::span<const multi::Multiaddress>());

    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &));

    MOCK_METHOD3(setProtocolHandler,
                 void(std::string_view,
                      const std::function<connection::Stream::Handler> &,
                      const std::function<bool(const peer::Protocol &)> &));

    MOCK_METHOD1(connect, void(const peer::PeerInfo &));

    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &, const peer::Protocol &,
                      const StreamResultHandler &));

    MOCK_CONST_METHOD0(getNetwork, network::Network &());

    MOCK_CONST_METHOD0(getPeerRepository, peer::PeerRepository &());

    MOCK_CONST_METHOD0(getRouter, const network::Router &());
  };
}  // namespace libp2p

#endif  // KAGOME_HOST_MOCK_HPP

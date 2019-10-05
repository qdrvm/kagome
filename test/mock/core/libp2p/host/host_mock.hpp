/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_MOCK_HPP
#define KAGOME_HOST_MOCK_HPP

#include <vector>

#include <gmock/gmock.h>
#include "libp2p/host/host.hpp"

namespace libp2p {
  class HostMock : public Host {
   public:
    ~HostMock() override = default;

    MOCK_CONST_METHOD0(getLibp2pVersion, std::string_view());
    MOCK_CONST_METHOD0(getLibp2pClientVersion, std::string_view());
    MOCK_CONST_METHOD0(getId, peer::PeerId());
    MOCK_CONST_METHOD0(getPeerInfo, peer::PeerInfo());
    MOCK_CONST_METHOD0(getAddresses, std::vector<multi::Multiaddress>());
    MOCK_CONST_METHOD0(getAddressesInterfaces,
                       std::vector<multi::Multiaddress>());
    MOCK_CONST_METHOD0(getObservedAddresses, std::vector<multi::Multiaddress>());
    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &));
    MOCK_METHOD3(setProtocolHandler,
                 void(const peer::Protocol&,
                      const std::function<connection::Stream::Handler> &,
                      const std::function<bool(const peer::Protocol &)> &));
    MOCK_METHOD1(connect, void(const peer::PeerInfo &));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &p, const peer::Protocol &protocol,
                      const StreamResultHandler &handler));
    MOCK_METHOD1(listen, outcome::result<void>(const multi::Multiaddress &ma));
    MOCK_METHOD1(closeListener,
                 outcome::result<void>(const multi::Multiaddress &ma));
    MOCK_METHOD1(removeListener,
                 outcome::result<void>(const multi::Multiaddress &ma));
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(getNetwork, network::Network &());
    MOCK_METHOD0(getPeerRepository, peer::PeerRepository &());
    MOCK_METHOD0(getRouter, network::Router &());
    MOCK_METHOD0(getBus, event::Bus &());
  };
}  // namespace libp2p

#endif  // KAGOME_HOST_MOCK_HPP

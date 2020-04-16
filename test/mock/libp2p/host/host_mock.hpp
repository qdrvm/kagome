/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBP2P_HOST_MOCK_HPP
#define LIBP2P_HOST_MOCK_HPP

#include <vector>

#include <gmock/gmock.h>
#include <libp2p/host/host.hpp>

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
    MOCK_CONST_METHOD0(getObservedAddresses,
                       std::vector<multi::Multiaddress>());
    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &));
    MOCK_METHOD3(setProtocolHandler,
                 void(const peer::Protocol &,
                      const std::function<connection::Stream::Handler> &,
                      const std::function<bool(const peer::Protocol &)> &));
    MOCK_METHOD1(connect, void(const peer::PeerInfo &));
    MOCK_METHOD3(newStream,
                 void(const peer::PeerInfo &p,
                      const peer::Protocol &protocol,
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

#endif  // LIBP2P_HOST_MOCK_HPP

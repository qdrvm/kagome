/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LISTENER_MOCK_HPP
#define KAGOME_LISTENER_MOCK_HPP

#include "libp2p/network/listener.hpp"

#include <gmock/gmock.h>

namespace libp2p::network {
  struct ListenerMock : public Listener {
    MOCK_CONST_METHOD0(isStarted, bool());

    MOCK_METHOD0(start, void());

    MOCK_METHOD0(stop, void());

    MOCK_METHOD1(closeListener,
                 outcome::result<void>(const multi::Multiaddress &));

    MOCK_METHOD1(listen, outcome::result<void>(const multi::Multiaddress &));

    MOCK_CONST_METHOD0(getListenAddresses, std::vector<multi::Multiaddress>());

    MOCK_CONST_METHOD0(getListenAddressesInterfaces,
                       outcome::result<std::vector<multi::Multiaddress>>());

    MOCK_METHOD1(handleProtocol, void(std::shared_ptr<protocol::BaseProtocol>));

    MOCK_METHOD2(setProtocolHandler,
                 void(const peer::Protocol &, StreamResultFunc));

    MOCK_METHOD3(setProtocolHandler,
                 void(const peer::Protocol &, StreamResultFunc,
                      Router::ProtoPredicate));
  };
}  // namespace libp2p::network

#endif  // KAGOME_LISTENER_MOCK_HPP

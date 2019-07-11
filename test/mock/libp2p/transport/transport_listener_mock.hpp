/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_LISTENER_MOCK_HPP
#define KAGOME_TRANSPORT_LISTENER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::transport {

  struct TransportListenerMock : public TransportListener {
    ~TransportListenerMock() override = default;

    MOCK_METHOD1(listen, outcome::result<void>(const multi::Multiaddress &));

    MOCK_CONST_METHOD1(canListen, bool(const multi::Multiaddress &address));

    MOCK_CONST_METHOD0(getListenMultiaddr,
                       outcome::result<multi::Multiaddress>());

    MOCK_CONST_METHOD0(isClosed, bool());

    MOCK_METHOD0(close, outcome::result<void>());
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_LISTENER_MOCK_HPP

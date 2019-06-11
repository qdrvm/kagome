/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_MOCK_HPP
#define KAGOME_TRANSPORT_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/transport/transport.hpp"

namespace libp2p::transport {
  class TransportMock : public Transport {
   public:
    MOCK_METHOD2(dial, void(multi::Multiaddress, HandlerFunc));

    MOCK_METHOD1(
        createListener,
        std::shared_ptr<TransportListener>(TransportListener::HandlerFunc));

    MOCK_CONST_METHOD1(canDial, bool(const multi::Multiaddress &));
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_MOCK_HPP

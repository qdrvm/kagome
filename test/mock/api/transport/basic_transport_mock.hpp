/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP
#define KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP

#include <gmock/gmock.h>
#include "api/transport/basic_transport.hpp"

namespace kagome::api {
  class BasicTransportMock : public BasicTransport {
   public:
    ~BasicTransportMock() override = default;

    BasicTransportMock(NetworkAddress address, uint16_t port)
        : address_{std::move(address)}, port_{port} {}

    MOCK_METHOD0(start, void());

    MOCK_METHOD0(stop, void());

    MOCK_METHOD1(processResponse, void(std::string));

   private:
    NetworkAddress address_;
    uint16_t port_;
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP

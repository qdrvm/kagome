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

    explicit BasicTransportMock(NetworkAddress address, uint16_t port)
        : address_{std::move(address)}, port_{port} {}

    MOCK_METHOD0(start, outcome::result<void>());

    MOCK_METHOD0(stop, void());

    void doRequest(std::string_view request) {
      dataReceived()(std::string(request));
    }

    MOCK_METHOD1(processResponse, void(const std::string &));

   private:
    NetworkAddress address_;
    uint16_t port_;
  };
}  // namespace kagome::service

#endif  // KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP

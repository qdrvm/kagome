/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP
#define KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP

#include <gmock/gmock.h>
#include "api/extrinsic/json_transport.hpp"

namespace kagome::api {
  class JsonTransportMock : public JsonTransport {
   public:
    ~JsonTransportMock() override = default;

    explicit JsonTransportMock(NetworkAddress address)
        : address_{std::move(address)} {}

    MOCK_METHOD0(start, outcome::result<void>());

    void stop() override {}

    void doRequest(std::string_view request) {
      dataReceived()(std::string(request));
    }

    MOCK_METHOD1(processResponse, void(const std::string &));

   private:
    NetworkAddress address_;
  };
}  // namespace kagome::service

#endif  // KAGOME_TEST_MOCK_API_EXTRINSIC_JSON_TRANSPORT_MOCK_HPP

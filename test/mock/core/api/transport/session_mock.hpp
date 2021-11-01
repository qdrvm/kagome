/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP
#define KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP

#include "api/transport/session.hpp"

namespace kagome::api {
  class SessionMock : public Session {
   public:
    ~SessionMock() override = default;

    MOCK_METHOD(Socket &, socket, (), (override));

    MOCK_METHOD(void, start, (), (override));

    MOCK_METHOD(void, respond, (std::string_view), (override));
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP

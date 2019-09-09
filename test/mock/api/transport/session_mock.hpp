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
    MOCK_METHOD0(start, void());
    MOCK_METHOD1(respond, void(std::string_view));
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_SESSION_MOCK_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP
#define KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP

#include <gmock/gmock.h>
#include "api/transport/listener.hpp"

namespace kagome::api {

  class ListenerMock : public Listener {
   public:
    ~ListenerMock() override = default;
    MOCK_METHOD1(start, void(NewSessionHandler));
    MOCK_METHOD0(stop, void(void));
    MOCK_METHOD1(doAccept, void(NewSessionHandler));
  };

}  // namespace kagome::server

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP

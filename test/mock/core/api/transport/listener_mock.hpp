/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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

    MOCK_METHOD(void, prepare, (), (override));

    MOCK_METHOD(void, start, (), (override));

    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(void, setHandlerForNewSession, (NewSessionHandler), (override));
    void setHandlerForNewSession(NewSessionHandler &&handler) override {
      setHandlerForNewSession(handler);
    }

    MOCK_METHOD(void, acceptOnce, (), (override));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP

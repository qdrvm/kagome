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
    MOCK_METHOD0(prepare, void());
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());

    void setHandlerForNewSession(NewSessionHandler &&handler) override {
      setHandlerForNewSession_rv(std::move(handler));
    }
    MOCK_METHOD1(setHandlerForNewSession_rv, void(NewSessionHandler));

    MOCK_METHOD0(acceptOnce, void());
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_TRANSPORT_LISTENER_MOCK_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::application {

  class AppStateManagerMock : public AppStateManager {
   public:
    MOCK_METHOD(void, atPrepare, (OnPrepare), ());
    void atPrepare(OnPrepare &&cb) override {
      atPrepare(cb);
    }

    MOCK_METHOD(void, atLaunch, (OnLaunch), ());
    void atLaunch(OnLaunch &&cb) override {
      atLaunch(cb);
    }

    MOCK_METHOD(void, atShutdown, (OnShutdown), ());
    void atShutdown(OnShutdown &&cb) override {
      atShutdown(cb);
    }

    MOCK_METHOD(void, run, (), (override));

    MOCK_METHOD(void, shutdown, (), (override));

    MOCK_METHOD(void, doPrepare, (), (override));

    MOCK_METHOD(void, doLaunch, (), (override));

    MOCK_METHOD(void, doShutdown, (), (override));

    MOCK_METHOD(State, state, (), (const, override));
  };

}  // namespace kagome::application

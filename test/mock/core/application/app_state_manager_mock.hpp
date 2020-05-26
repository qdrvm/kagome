/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_STATE_MANAGER_MOCK
#define KAGOME_APP_STATE_MANAGER_MOCK

#include "application/app_state_manager.hpp"

#include <gmock/gmock.h>

namespace kagome {

  class AppStateManagerMock : public AppStateManager {
   public:
    void atPrepare(Callback &&cb) {
      atPrepare(cb);
    }
    MOCK_METHOD1(atPrepare, void(Callback));

    void atLaunch(Callback &&cb) {
      atLaunch(cb);
    }
    MOCK_METHOD1(atLaunch, void(Callback));

    void atShuttingdown(Callback &&cb) {
      atShuttingdown(cb);
    }
    MOCK_METHOD1(atShuttingdown, void(Callback));

    MOCK_METHOD0(run, void());
    MOCK_METHOD0(shutdown, void());

    MOCK_METHOD0(prepare, void());
    MOCK_METHOD0(launch, void());
    MOCK_METHOD0(shuttingdown, void());

    MOCK_CONST_METHOD0(state, State());
  };

}  // namespace kagome

#endif  // KAGOME_APP_STATE_MANAGER_MOCK

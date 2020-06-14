/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_STATE_MANAGER_MOCK
#define KAGOME_APP_STATE_MANAGER_MOCK

#include "application/app_state_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::application {

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

    void atShutdown(Callback &&cb) {
      atShutdown(cb);
    }
    MOCK_METHOD1(atShutdown, void(Callback));

    MOCK_METHOD0(run, void());
    MOCK_METHOD0(shutdown, void());

    MOCK_METHOD0(doPrepare, void());
    MOCK_METHOD0(doLaunch, void());
    MOCK_METHOD0(doShutdown, void());

    MOCK_CONST_METHOD0(state, State());
  };

}  // namespace kagome

#endif  // KAGOME_APP_STATE_MANAGER_MOCK

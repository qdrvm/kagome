/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_DISPATCHER
#define KAGOME_APPLICATION_DISPATCHER

#include "common/logger.hpp"

namespace kagome {

  class AppStateManager : public std::enable_shared_from_this<AppStateManager> {
   public:
    using Callback = std::function<void()>;

    enum class State {
      Init,
      Prepare,
      Cancel,
      ReadyToStart,
      Starting,
      Works,
      ShuttingDown,
      ReadyToStop,
    };

    virtual ~AppStateManager() = default;

    virtual bool atPrepare(Callback &&cb) = 0;
    virtual bool atLaunch(Callback &&cb) = 0;
    virtual bool atShuttingdown(Callback &&cb) = 0;

    bool reg(Callback &&prepare_cb,
             Callback &&launch_cb,
             Callback &&shuttingdown_cb) {
      return atPrepare(std::move(prepare_cb)) && atLaunch(std::move(launch_cb))
             && atShuttingdown(std::move(shuttingdown_cb));
    }

    virtual void run() = 0;
    virtual void shutdown() = 0;

   protected:
    virtual void prepare() = 0;
    virtual void launch() = 0;
    virtual void shuttingdown() = 0;
  };

}  // namespace kagome

#endif  // KAGOME_APPLICATION_DISPATCHER

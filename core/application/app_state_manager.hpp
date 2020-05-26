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

    virtual void atPrepare(Callback &&cb) = 0;
    virtual void atLaunch(Callback &&cb) = 0;
    virtual void atShuttingdown(Callback &&cb) = 0;

    void reg(Callback &&prepare_cb,
             Callback &&launch_cb,
             Callback &&shuttingdown_cb) {
      atPrepare(std::move(prepare_cb));
      atLaunch(std::move(launch_cb));
      atShuttingdown(std::move(shuttingdown_cb));
    }

    virtual void run() = 0;
    virtual void shutdown() = 0;

    virtual State state() const = 0;

   protected:
    virtual void prepare() = 0;
    virtual void launch() = 0;
    virtual void shuttingdown() = 0;
  };

  struct AppStateException : public std::runtime_error {
    explicit AppStateException(std::string message)
        : std::runtime_error("Wrong workflow at " + std::move(message)) {}
  };
}  // namespace kagome

#endif  // KAGOME_APPLICATION_DISPATCHER

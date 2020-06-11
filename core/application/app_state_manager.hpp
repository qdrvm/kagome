/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_DISPATCHER
#define KAGOME_APPLICATION_DISPATCHER

#include "common/logger.hpp"

namespace kagome::application {

  class AppStateManager : public std::enable_shared_from_this<AppStateManager> {
   public:
    using Callback = std::function<void()>;

    enum class State {
      Init,
      Prepare,
      ReadyToStart,
      Starting,
      Works,
      ShuttingDown,
      ReadyToStop,
    };

    virtual ~AppStateManager() = default;

    virtual void atPrepare(Callback &&cb) = 0;
    virtual void atLaunch(Callback &&cb) = 0;
    virtual void atShutdown(Callback &&cb) = 0;

    void registerHandlers(Callback &&prepare_cb,
                          Callback &&launch_cb,
                          Callback &&shutdown_cb) {
      atPrepare(std::move(prepare_cb));
      atLaunch(std::move(launch_cb));
      atShutdown(std::move(shutdown_cb));
    }

    template <typename Controlled>
    void takeControl(Controlled &entity) {
      registerHandlers([&entity] { entity.prepare(); },
                       [&entity] { entity.start(); },
                       [&entity] { entity.stop(); });
    }

    virtual void run() = 0;
    virtual void shutdown() = 0;

    virtual State state() const = 0;

   protected:
    virtual void doPrepare() = 0;
    virtual void doLaunch() = 0;
    virtual void doShutdown() = 0;
  };

  struct AppStateException : public std::runtime_error {
    explicit AppStateException(std::string message)
        : std::runtime_error("Wrong workflow at " + std::move(message)) {}
  };
}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_DISPATCHER

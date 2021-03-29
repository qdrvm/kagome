/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_DISPATCHER
#define KAGOME_APPLICATION_DISPATCHER

#include "log/logger.hpp"

namespace kagome::application {

  class AppStateManager : public std::enable_shared_from_this<AppStateManager> {
   public:
    using OnPrepare = std::function<bool()>;
    using OnLaunch = std::function<bool()>;
    using OnShutdown = std::function<void()>;

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

    /**
     * @brief Execute \param cb at stage of prepare application
     * @param cb
     */
    virtual void atPrepare(OnPrepare &&cb) = 0;

    /**
     * @brief Execute \param cb immediately before start application
     * @param cb
     */
    virtual void atLaunch(OnLaunch &&cb) = 0;

    /**
     * @brief Execute \param cb at stage of shutting down application
     * @param cb
     */
    virtual void atShutdown(OnShutdown &&cb) = 0;

    /**
     * @brief Registration of all stages' handlers at the same time
     * @param prepare_cb - handler for stage of prepare
     * @param launch_cb - handler for doing immediately before start application
     * @param shutdown_cb - handler for stage of shutting down application
     */
    void registerHandlers(OnPrepare &&prepare_cb,
                          OnLaunch &&launch_cb,
                          OnShutdown &&shutdown_cb) {
      atPrepare(std::move(prepare_cb));
      atLaunch(std::move(launch_cb));
      atShutdown(std::move(shutdown_cb));
    }

    /**
     * @brief Registration special methods of object as handlers
     * for stages of application life-cycle
     * @param entity is registered entity
     */
    template <typename Controlled>
    void takeControl(Controlled &entity) {
      registerHandlers([&entity]() -> bool { return entity.prepare(); },
                       [&entity]() -> bool { return entity.start(); },
                       [&entity]() -> void { return entity.stop(); });
    }

    /// Start application life cycle
    virtual void run() = 0;

    /// Initiate shutting down (at any time)
    virtual void shutdown() = 0;

    /// Get current stage
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

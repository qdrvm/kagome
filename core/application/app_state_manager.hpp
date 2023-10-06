/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_DISPATCHER
#define KAGOME_APPLICATION_DISPATCHER

#include "log/logger.hpp"

namespace kagome::application {

  class AppStateManager : public std::enable_shared_from_this<AppStateManager> {
   public:
    using OnInject = std::function<bool()>;
    using OnPrepare = std::function<bool()>;
    using OnLaunch = std::function<bool()>;
    using OnShutdown = std::function<void()>;

    enum class State {
      Init,
      Injecting,
      Injected,
      Prepare,
      ReadyToStart,
      Starting,
      Works,
      ShuttingDown,
      ReadyToStop,
    };

    virtual ~AppStateManager() = default;

    /**
     * @brief Execute \param cb at stage 'injections' of application
     * @param cb
     */
    virtual void atInject(OnInject &&cb) = 0;

    /**
     * @brief Execute \param cb at stage 'preparations' of application
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

   private:
    template <typename C, typename = int>
    struct HasMethodInject : std::false_type {};
    template <typename C>
    struct HasMethodInject<C, decltype(&C::inject, 0)> : std::true_type {};

    template <typename C, typename = int>
    struct HasMethodPrepare : std::false_type {};
    template <typename C>
    struct HasMethodPrepare<C, decltype(&C::prepare, 0)> : std::true_type {};

    template <typename C, typename = int>
    struct HasMethodStart : std::false_type {};
    template <typename C>
    struct HasMethodStart<C, decltype(&C::start, 0)> : std::true_type {};

    template <typename C, typename = int>
    struct HasMethodStop : std::false_type {};
    template <typename C>
    struct HasMethodStop<C, decltype(&C::stop, 0)> : std::true_type {};

   public:
    /**
     * @brief Registration special methods (if any) of object as handlers
     * for stages of application life-cycle
     * @param entity is registered entity
     */
    template <typename Controlled>
    void takeControl(Controlled &entity) {
      if constexpr (HasMethodInject<Controlled>::value) {
        atInject([&entity]() -> bool { return entity.inject(); });
      }
      if constexpr (HasMethodPrepare<Controlled>::value) {
        atPrepare([&entity]() -> bool { return entity.prepare(); });
      }
      if constexpr (HasMethodStart<Controlled>::value) {
        atLaunch([&entity]() -> bool { return entity.start(); });
      }
      if constexpr (HasMethodStop<Controlled>::value) {
        atShutdown([&entity]() -> void { return entity.stop(); });
      }
    }

    /// Start application life cycle
    virtual void run() = 0;

    /// Initiate shutting down (at any time)
    virtual void shutdown() = 0;

    /// Get current stage
    virtual State state() const = 0;

   protected:
    virtual void doInject() = 0;
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

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"

namespace kagome::application {

  // concepts that check if an object has a method that is called by app state
  // manager. Deliberately avoid checking that the method returns bool,
  // because if there's a method with an appropriate name, and it doesn't return
  // bool, we want it to be a compile error instead of silently ignoring it
  // because the concept is not satisfied.
  template <typename T>
  concept AppStateInjectable = requires(T& t) { t.inject(); };
  template <typename T>
  concept AppStatePreparable = requires(T& t) { t.prepare(); };
  template <typename T>
  concept AppStateStartable = requires(T& t) { t.start(); };
  template <typename T>
  concept AppStateStoppable = requires(T& t) { t.stop(); };

  // if an object is registered with AppStateManager but has no method
  // that is called by AppStateManager, there's probably something wrong
  template <typename T>
  concept AppStateControllable = AppStatePreparable<T> || AppStateInjectable<T>
                              || AppStateStoppable<T> || AppStateStartable<T>;

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

   public:
    /**
     * @brief Registration special methods (if any) of object as handlers
     * for stages of application life-cycle
     * @param entity is registered entity
     */
    template <AppStateControllable Controlled>
    void takeControl(Controlled &entity) {
      if constexpr (AppStateInjectable<Controlled>) {
        atInject([&entity]() -> bool { return entity.inject(); });
      }
      if constexpr (AppStatePreparable<Controlled>) {
        atPrepare([&entity]() -> bool { return entity.prepare(); });
      }
      if constexpr (AppStateStartable<Controlled>) {
        atLaunch([&entity]() -> bool { return entity.start(); });
      }
      if constexpr (AppStateStoppable<Controlled>) {
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

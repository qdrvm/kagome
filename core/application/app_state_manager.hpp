/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <stdexcept>
#include <string>

namespace kagome::application {

  // concepts that check if an object has a method that is called by app state
  // manager. Deliberately avoid checking that the method returns bool,
  // because if there's a method with an appropriate name, and it doesn't return
  // bool, we want it to be a compile error instead of silently ignoring it
  // because the concept is not satisfied.
  template <typename T>
  concept AppStatePreparable = requires(T &t) { t.prepare(); };
  template <typename T>
  concept AppStateStartable = requires(T &t) { t.start(); };
  template <typename T>
  concept AppStateStoppable = requires(T &t) { t.stop(); };

  // if an object is registered with AppStateManager but has no method
  // that is called by AppStateManager, there's probably something wrong
  template <typename T>
  concept AppStateControllable =
      AppStatePreparable<T> || AppStateStoppable<T> || AppStateStartable<T>;

  template <typename T>
  concept ActionRetBool = std::same_as<std::invoke_result_t<T>, bool>;

  template <typename T>
  concept ActionRetVoid = std::is_void_v<std::invoke_result_t<T>>;

  class Action {
   public:
    Action(ActionRetBool auto &&f)
        : f_([f = std::move(f)]() mutable { return f(); }) {}

    Action(ActionRetVoid auto &&f)
        : f_([f = std::move(f)]() mutable { return f(), true; }) {}

    bool operator()() {
      return f_();
    }

   private:
    std::function<bool()> f_;
  };

  class AppStateManager {
   public:
    using OnPrepare = Action;
    using OnLaunch = Action;
    using OnShutdown = Action;

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
      if constexpr (AppStatePreparable<Controlled>) {
        atPrepare([&entity] { return entity.prepare(); });
      }
      if constexpr (AppStateStartable<Controlled>) {
        atLaunch([&entity] { return entity.start(); });
      }
      if constexpr (AppStateStoppable<Controlled>) {
        atShutdown([&entity] { return entity.stop(); });
      }
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

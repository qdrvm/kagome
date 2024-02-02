/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_state_manager_impl.hpp"

#include <csignal>
#include <functional>

namespace kagome::application {
  std::atomic_bool AppStateManagerImpl::signals_enabled{false};

  void AppStateManagerImpl::signalsEnable() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = shuttingDownSignalsHandler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &act.sa_mask, nullptr);
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);
    signals_enabled.store(true);
    sigprocmask(SIG_UNBLOCK, &act.sa_mask, nullptr);
  }

  void AppStateManagerImpl::signalsDisable() {
    auto expected = true;
    if (not signals_enabled.compare_exchange_strong(expected, false)) {
      return;
    }
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);
  }

  std::weak_ptr<AppStateManagerImpl> AppStateManagerImpl::wp_to_myself;

  void AppStateManagerImpl::shuttingDownSignalsHandler(int signal) {
    signalsDisable();
    if (auto self = wp_to_myself.lock()) {
      SL_TRACE(self->logger_, "Shutting down requested by signal {}", signal);
      self->shutdown();
      SL_TRACE(self->logger_, "Shutting down request handled");
    }
  }

  AppStateManagerImpl::AppStateManagerImpl()
      : logger_(log::createLogger("AppStateManager", "application")) {
    signalsEnable();
  }

  AppStateManagerImpl::~AppStateManagerImpl() {
    signalsDisable();
    wp_to_myself.reset();
  }

  void AppStateManagerImpl::reset() {
    std::lock_guard lg(mutex_);

    std::queue<OnInject> empty_inject;
    std::swap(inject_, empty_inject);

    std::queue<OnPrepare> empty_prepare;
    std::swap(prepare_, empty_prepare);

    std::queue<OnLaunch> empty_launch;
    std::swap(launch_, empty_launch);

    std::queue<OnShutdown> empty_shutdown;
    std::swap(shutdown_, empty_shutdown);

    state_ = State::Init;
    shutdown_requested_ = false;
  }

  void AppStateManagerImpl::atInject(OnInject &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ != State::Init and state_ != State::Injecting) {
      throw AppStateException("adding callback for stage 'inject'");
    }
    inject_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atPrepare(OnPrepare &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ > State::Prepare) {
      throw AppStateException("adding callback for stage 'prepare'");
    }
    prepare_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atLaunch(OnLaunch &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ > State::Starting) {
      throw AppStateException("adding callback for stage 'launch'");
    }
    launch_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atShutdown(OnShutdown &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ > State::ShuttingDown) {
      throw AppStateException("adding callback for stage 'shutdown'");
    }
    shutdown_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::doInject() {
    std::lock_guard lg(mutex_);
    if (state_ != State::Init and state_ != State::Injecting) {
      throw AppStateException("running stage 'injecting'");
    }
    state_ = State::Injecting;

    while (!inject_.empty()) {
      auto &cb = inject_.front();
      if (state_ == State::Injecting) {
        auto success = cb();
        if (not success) {
          state_ = State::ShuttingDown;
        }
      }
      inject_.pop();
    }

    if (state_ == State::Injecting) {
      state_ = State::Injected;
    } else {
      shutdown();
    }
  }

  void AppStateManagerImpl::doPrepare() {
    std::lock_guard lg(mutex_);
    if (state_ != State::Injected) {
      throw AppStateException("running stage 'prepare'");
    }
    state_ = State::Prepare;

    while (!prepare_.empty()) {
      auto &cb = prepare_.front();
      if (state_ == State::Prepare) {
        auto success = cb();
        if (not success) {
          SL_ERROR(logger_, "Preparation stage failed");
          state_ = State::ShuttingDown;
        }
      }
      prepare_.pop();
    }

    if (state_ == State::Prepare) {
      state_ = State::ReadyToStart;
    } else {
      shutdown();
    }
  }

  void AppStateManagerImpl::doLaunch() {
    std::lock_guard lg(mutex_);
    if (state_ != State::ReadyToStart) {
      throw AppStateException("running stage 'launch'");
    }
    state_ = State::Starting;

    while (!launch_.empty()) {
      auto &cb = launch_.front();
      if (state_ == State::Starting) {
        auto success = cb();
        if (not success) {
          state_ = State::ShuttingDown;
        }
      }
      launch_.pop();
    }

    if (state_ == State::Starting) {
      state_ = State::Works;
    } else {
      shutdown();
    }
  }

  void AppStateManagerImpl::doShutdown() {
    std::lock_guard lg(mutex_);
    if (state_ == State::ReadyToStop) {
      return;
    }

    std::queue<OnInject> empty_inject;
    std::swap(inject_, empty_inject);

    std::queue<OnPrepare> empty_prepare;
    std::swap(prepare_, empty_prepare);

    std::queue<OnLaunch> empty_launch;
    std::swap(launch_, empty_launch);

    state_ = State::ShuttingDown;

    while (!shutdown_.empty()) {
      auto &cb = shutdown_.front();
      cb();
      shutdown_.pop();
    }

    state_ = State::ReadyToStop;
  }

  void AppStateManagerImpl::run() {
    wp_to_myself = weak_from_this();
    if (wp_to_myself.expired()) {
      throw std::logic_error(
          "AppStateManager must be instantiated on shared pointer before run");
    }

    doInject();

    if (state_ == State::Injected) {
      doPrepare();
    }

    if (state_ == State::ReadyToStart) {
      doLaunch();
    }

    SL_TRACE(logger_, "Start waiting shutdown request...");
    shutdownRequestWaiting();

    SL_TRACE(logger_, "Start doing shutdown...");
    doShutdown();
    SL_TRACE(logger_, "Shutdown is done");
  }

  void AppStateManagerImpl::shutdownRequestWaiting() {
    std::unique_lock lock(cv_mutex_);
    cv_.wait(lock, [&] { return shutdown_requested_.load(); });
  }

  void AppStateManagerImpl::shutdown() {
    signalsDisable();
    std::lock_guard lg(cv_mutex_);
    shutdown_requested_ = true;
    cv_.notify_one();
  }
}  // namespace kagome::application

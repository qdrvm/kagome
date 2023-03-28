/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_state_manager_impl.hpp"

#include <csignal>
#include <functional>

namespace kagome::application {

  std::weak_ptr<AppStateManager> AppStateManagerImpl::wp_to_myself;

  void AppStateManagerImpl::shuttingDownSignalsHandler(int) {
    if (auto self = wp_to_myself.lock()) {
      self->shutdown();
    }
  }

  AppStateManagerImpl::AppStateManagerImpl()
      : logger_(log::createLogger("AppStateManager", "application")) {
    struct sigaction act {};
    memset(&act, 0, sizeof(act));
    act.sa_handler = shuttingDownSignalsHandler;  // NOLINT
    sigset_t set;                                 // NOLINT
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);
    act.sa_mask = set;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);
    sigprocmask(SIG_UNBLOCK, &act.sa_mask, nullptr);
  }

  AppStateManagerImpl::~AppStateManagerImpl() {
    struct sigaction act {};
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;  // NOLINT
    sigset_t set;              // NOLINT
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);
    act.sa_mask = set;
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);
    sigaction(SIGQUIT, &act, nullptr);
  }

  void AppStateManagerImpl::reset() {
    std::lock_guard lg(mutex_);
    while (!inject_.empty()) {
      inject_.pop();
    }
    while (!prepare_.empty()) {
      prepare_.pop();
    }
    while (!launch_.empty()) {
      launch_.pop();
    }
    while (!shutdown_.empty()) {
      shutdown_.pop();
    }
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
    if (state_ > State::Injected) {
      throw AppStateException("adding callback for stage 'prepare'");
    }
    prepare_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atLaunch(OnLaunch &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ > State::ReadyToStart) {
      throw AppStateException("adding callback for stage 'launch'");
    }
    launch_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atShutdown(OnShutdown &&cb) {
    std::lock_guard lg(mutex_);
    if (state_ > State::Works) {
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

    while (!inject_.empty()) {
      inject_.pop();
    }

    while (!prepare_.empty()) {
      prepare_.pop();
    }

    while (!launch_.empty()) {
      launch_.pop();
    }

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

    std::unique_lock lock(cv_mutex_);
    cv_.wait(lock, [&] { return shutdown_requested_.load(); });

    doShutdown();
  }

  void AppStateManagerImpl::shutdown() {
    shutdown_requested_ = true;
    std::lock_guard lg(cv_mutex_);
    cv_.notify_one();
  }
}  // namespace kagome::application

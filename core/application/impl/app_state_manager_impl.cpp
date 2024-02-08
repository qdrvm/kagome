/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_state_manager_impl.hpp"

#include <csignal>
#include <functional>

namespace kagome::application {

  std::weak_ptr<AppStateManagerImpl> AppStateManagerImpl::wp_to_myself;

  void AppStateManagerImpl::shuttingDownSignalsHandler(int signal) {
    if (auto self = wp_to_myself.lock()) {
      SL_TRACE(self->logger_, "Shutdown signal {} received", signal);
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
    SL_TRACE(logger_, "Signal handler set up");
  }

  AppStateManagerImpl::~AppStateManagerImpl() {
    wp_to_myself.reset();

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

    std::queue<OnInject> empty_inject;
    std::swap(inject_, empty_inject);

    std::queue<OnPrepare> empty_prepare;
    std::swap(prepare_, empty_prepare);

    std::queue<OnLaunch> empty_launch;
    std::swap(launch_, empty_launch);

    std::queue<OnShutdown> empty_shutdown;
    std::swap(shutdown_, empty_shutdown);

    state_ = State::Init;
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

    if (not inject_.empty()) {
      SL_TRACE(logger_, "Running stage 'injecting'…");
    }

    while (not inject_.empty()) {
      auto &cb = inject_.front();
      if (state_.load() == State::Injecting) {
        auto success = cb();
        if (not success) {
          SL_ERROR(logger_, "Stage 'injecting' is failed");
          State s = State::Injecting;
          state_.compare_exchange_strong(s, State::ShuttingDown);
        }
      }
      inject_.pop();
    }

    State s = State::Injecting;
    state_.compare_exchange_strong(s, State::Injected);
  }

  void AppStateManagerImpl::doPrepare() {
    std::lock_guard lg(mutex_);

    State state = State::Injected;
    if (not state_.compare_exchange_strong(state, State::Prepare)) {
      if (state == State::ShuttingDown or state == State::ReadyToStop) {
        return;
      }
      throw AppStateException("running stage 'preparing'");
    }

    if (not prepare_.empty()) {
      SL_TRACE(logger_, "Running stage 'preparing'…");
    }

    while (!prepare_.empty()) {
      auto &cb = prepare_.front();
      if (state_ == State::Prepare) {
        auto success = cb();
        if (not success) {
          SL_ERROR(logger_, "Stage 'preparing' is failed");
          state = State::Prepare;
          state_.compare_exchange_strong(state, State::ShuttingDown);
        }
      }
      prepare_.pop();
    }

    state = State::Prepare;
    state_.compare_exchange_strong(state, State::ReadyToStart);
  }

  void AppStateManagerImpl::doLaunch() {
    std::lock_guard lg(mutex_);

    State state = State::ReadyToStart;
    if (not state_.compare_exchange_strong(state, State::Starting)) {
      if (state == State::ShuttingDown or state == State::ReadyToStop) {
        return;
      }
      throw AppStateException("running stage 'launch'");
    }

    if (not launch_.empty()) {
      SL_TRACE(logger_, "Running stage 'launch'…");
    }

    while (!launch_.empty()) {
      auto &cb = launch_.front();
      if (state_.load() == State::Starting) {
        auto success = cb();
        if (not success) {
          SL_ERROR(logger_, "Stage 'launch' is failed");
          state = State::Starting;
          state_.compare_exchange_strong(state, State::ShuttingDown);
        }
      }
      launch_.pop();
    }

    state = State::Starting;
    state_.compare_exchange_strong(state, State::Works);
  }

  void AppStateManagerImpl::doShutdown() {
    std::lock_guard lg(mutex_);

    State state = State::ShuttingDown;
    if (not state_.compare_exchange_strong(state, State::ShuttingDown)) {
      if (state == State::ReadyToStop) {
        return;
      }
      throw AppStateException("running stage 'shutting down'");
    }

    std::queue<OnInject> empty_inject;
    std::swap(inject_, empty_inject);

    std::queue<OnPrepare> empty_prepare;
    std::swap(prepare_, empty_prepare);

    std::queue<OnLaunch> empty_launch;
    std::swap(launch_, empty_launch);

    while (!shutdown_.empty()) {
      auto &cb = shutdown_.front();
      cb();
      shutdown_.pop();
    }

    state = State::ShuttingDown;
    state_.compare_exchange_strong(state, State::ReadyToStop);
  }

  void AppStateManagerImpl::run() {
    wp_to_myself = weak_from_this();
    if (wp_to_myself.expired()) {
      throw std::logic_error(
          "AppStateManager must be instantiated on shared pointer before run");
    }

    doInject();

    doPrepare();

    doLaunch();

    if (state_.load() == State::Works) {
      SL_TRACE(logger_, "All components started; waiting shutdown request…");
      shutdownRequestWaiting();
    }

    SL_TRACE(logger_, "Start doing shutdown…");
    doShutdown();
    SL_TRACE(logger_, "Shutdown is done");

    if (state_.load() != State::ReadyToStop) {
      throw std::logic_error(
          "AppStateManager is expected in stage 'ready to stop'");
    }
  }

  void AppStateManagerImpl::shutdownRequestWaiting() {
    std::unique_lock lock(cv_mutex_);
    cv_.wait(lock, [&] { return state_ == State::ShuttingDown; });
  }

  void AppStateManagerImpl::shutdown() {
    if (state_.load() == State::ReadyToStop) {
      SL_TRACE(logger_, "Shutting down requested, but app is ready to stop");
      return;
    }

    if (state_.load() == State::ShuttingDown) {
      SL_TRACE(logger_, "Shutting down requested, but it's in progress");
      return;
    }

    SL_TRACE(logger_, "Shutting down requested…");
    std::lock_guard lg(cv_mutex_);
    state_.store(State::ShuttingDown);
    cv_.notify_one();
  }
}  // namespace kagome::application

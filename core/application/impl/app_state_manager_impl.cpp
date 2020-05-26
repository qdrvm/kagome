/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_state_manager_impl.hpp"

#include <csignal>
#include <functional>

namespace kagome {

  std::weak_ptr<AppStateManager> AppStateManagerImpl::wp_to_myself;

  AppStateManagerImpl::AppStateManagerImpl()
      : logger_(common::createLogger("Application")) {
    struct sigaction act {};
    memset(&act, 0, sizeof(act));
    act.sa_handler = shuttingDownSignalsHandler;
    sigset_t set;
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
    act.sa_handler = SIG_DFL;
    sigset_t set;
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
    while (!prepare_.empty()) prepare_.pop();
    while (!launch_.empty()) launch_.pop();
    while (!shutdown_.empty()) shutdown_.pop();
    state_ = State::Init;
  }

  void AppStateManagerImpl::atPrepare(Callback &&cb) {
    if (state_ > State::Init) {
      throw AppStateException("adding callback for stage 'prepare'");
    }
    prepare_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atLaunch(Callback &&cb) {
    if (state_ > State::ReadyToStart) {
      throw AppStateException("adding callback for stage 'launch'");
    }
    launch_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::atShuttingdown(Callback &&cb) {
    if (state_ > State::Works) {
      throw AppStateException("adding callback for stage 'shutdown'");
    }
    shutdown_.emplace(std::move(cb));
  }

  void AppStateManagerImpl::prepare() {
    if (state_ != State::Init) {
      throw AppStateException("running stage 'prepare'");
    }
    state_ = State::Prepare;

    while (!prepare_.empty()) {
      auto &cb = prepare_.front();
      if (state_ == State::Prepare) {
        cb();
      }
      prepare_.pop();
    }

    if (state_ == State::Prepare) {
      state_ = State::ReadyToStart;
    }
    if (state_ == State::Cancel) {
      shutdown();
    }
  }

  void AppStateManagerImpl::launch() {
    if (state_ != State::ReadyToStart) {
      throw AppStateException("running stage 'launch'");
    }
    state_ = State::Starting;

    while (!launch_.empty()) {
      auto &cb = launch_.front();
      if (state_ != State::Cancel) {
        cb();
      }
      launch_.pop();
    }

    if (state_ == State::Starting) {
      state_ = State::Works;
    }
    if (state_ == State::Cancel) {
      shutdown();
    }
  }

  void AppStateManagerImpl::shuttingdown() {
    if (state_ == State::ReadyToStop) {
      return;
    }

    while (!prepare_.empty()) prepare_.pop();

    while (!launch_.empty()) launch_.pop();

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
          "AppStateManagerImpl must be instantiate on shared pointer before "
          "run");
    }

    prepare();
    launch();

    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return state_ == State::ShuttingDown; });

    shuttingdown();
  }

  void AppStateManagerImpl::shutdown() {
    state_ = State::ShuttingDown;
    cv_.notify_one();
  }

}  // namespace kagome

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"

#include <condition_variable>
#include <csignal>
#include <mutex>
#include <queue>

#include "log/logger.hpp"

namespace kagome::application {

  class AppStateManagerImpl
      : public AppStateManager,
        public std::enable_shared_from_this<AppStateManagerImpl> {
   public:
    AppStateManagerImpl();
    AppStateManagerImpl(const AppStateManagerImpl &) = delete;
    AppStateManagerImpl(AppStateManagerImpl &&) = delete;

    ~AppStateManagerImpl() override;

    AppStateManagerImpl &operator=(const AppStateManagerImpl &) = delete;
    AppStateManagerImpl &operator=(AppStateManagerImpl &&) = delete;

    void atPrepare(OnPrepare &&cb) override;
    void atLaunch(OnLaunch &&cb) override;
    void atShutdown(OnShutdown &&cb) override;

    void run() override;
    void shutdown() override;

    State state() const override {
      return state_;
    }

   protected:
    void reset();

    void doPrepare() override;
    void doLaunch() override;
    void doShutdown() override;

   private:
    static std::atomic_bool signals_enabled;
    static void signalsEnable();
    static void signalsDisable();
    static std::weak_ptr<AppStateManagerImpl> wp_to_myself;
    static void shuttingDownSignalsHandler(int);

    void shutdownRequestWaiting();

    log::Logger logger_;

    std::atomic<State> state_ = State::Init;

    std::recursive_mutex mutex_;

    std::mutex cv_mutex_;
    std::condition_variable cv_;

    std::queue<OnPrepare> prepare_;
    std::queue<OnLaunch> launch_;
    std::queue<OnShutdown> shutdown_;
  };

}  // namespace kagome::application

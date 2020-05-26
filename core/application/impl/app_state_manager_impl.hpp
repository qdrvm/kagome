/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_DISPATCHER_IMPL
#define KAGOME_APPLICATION_DISPATCHER_IMPL

#include "application/app_state_manager.hpp"

#include <condition_variable>
#include <csignal>
#include <mutex>
#include <queue>

#include "common/logger.hpp"

namespace kagome {

  class AppStateManagerImpl : public AppStateManager {
   public:
    AppStateManagerImpl();
	  AppStateManagerImpl(const AppStateManagerImpl&) = delete;
	  AppStateManagerImpl(AppStateManagerImpl&&) noexcept = delete;

    ~AppStateManagerImpl() override;

	  AppStateManagerImpl& operator=(AppStateManagerImpl const&) = delete;
	  AppStateManagerImpl& operator=(AppStateManagerImpl&&) noexcept = delete;

	  void atPrepare(Callback &&cb) override;
    void atLaunch(Callback &&cb) override;
    void atShuttingdown(Callback &&cb) override;

    void run() override;
    void shutdown() override;

    State state() const override {
      return state_;
    }

   protected:
    void reset();

    void prepare() override;
    void launch() override;
    void shuttingdown() override;

   private:
    static std::weak_ptr<AppStateManager> wp_to_myself;
    static void shuttingDownSignalsHandler(int) {
      if (auto self = wp_to_myself.lock()) {
        self->shutdown();
      }
    }

    common::Logger logger_;

    State state_ = State::Init;

    std::mutex mutex_;
    std::condition_variable cv_;

    std::queue<std::function<void()>> prepare_;
    std::queue<std::function<void()>> launch_;
    std::queue<std::function<void()>> shutdown_;
  };

}  // namespace kagome

#endif  // KAGOME_APPLICATION_DISPATCHER_IMPL

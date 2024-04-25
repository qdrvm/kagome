/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"
#include "log/logger.hpp"
#include "utils/pool_handler_ready.hpp"
#include "utils/thread_pool.hpp"

namespace kagome {
  auto poolHandlerReadyMake(auto *self,
                            std::shared_ptr<application::AppStateManager> app,
                            const ThreadPool &thread_pool,
                            const log::Logger &log) {
    auto thread = std::make_shared<PoolHandlerReady>(thread_pool.io_context());
    app->atLaunch([self,
                   weak_app{std::weak_ptr{app}},
                   log,
                   weak_thread{std::weak_ptr{thread}}] {
      auto thread = weak_thread.lock();
      if (not thread) {
        return;
      }
      thread->postAlways(
          [weak_self{self->weak_from_this()}, weak_app, log, weak_thread] {
            auto thread = weak_thread.lock();
            if (not thread) {
              return;
            }
            auto self = weak_self.lock();
            if (not self) {
              return;
            }
            if (self->tryStart()) {
              thread->setReady();
              return;
            }
            SL_CRITICAL(log, "start failed");
            if (auto app = weak_app.lock()) {
              app->shutdown();
            }
          });
    });
    app->takeControl(*thread);
    return thread;
  }

  inline auto poolHandlerReadyMake(application::AppStateManager &app,
                                   const ThreadPool &thread_pool) {
    auto thread = std::make_shared<PoolHandlerReady>(thread_pool.io_context());
    app.atLaunch([weak_thread{std::weak_ptr{thread}}] {
      auto thread = weak_thread.lock();
      if (not thread) {
        return;
      }
      thread->setReady();
    });
    app.takeControl(*thread);
    return thread;
  }
}  // namespace kagome

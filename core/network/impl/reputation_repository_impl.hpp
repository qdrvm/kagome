/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/reputation_repository.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

#include "aio/cancel.hpp"
#include "aio/timer.fwd.hpp"
#include "log/logger.hpp"

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::common {
  class MainThreadPool;
}  // namespace kagome::common

namespace kagome::network {

  class ReputationRepositoryImpl
      : public ReputationRepository,
        public std::enable_shared_from_this<ReputationRepositoryImpl> {
   public:
    ReputationRepositoryImpl(application::AppStateManager &app_state_manager,
                             common::MainThreadPool &main_thread_pool,
                             aio::TimerPtr scheduler);

    void start();

    Reputation reputation(const PeerId &peer_id) const override;

    Reputation change(const PeerId &peer_id, ReputationChange diff) override;

    Reputation changeForATime(const PeerId &peer_id,
                              ReputationChange diff,
                              std::chrono::seconds duration) override;

   private:
    void tick();

    mutable std::mutex mutex_;
    std::shared_ptr<PoolHandler> main_thread_;
    aio::TimerPtr scheduler_;
    std::unordered_map<PeerId, Reputation> reputation_table_;

    aio::Cancel tick_handler_;

    log::Logger log_;
  };

}  // namespace kagome::network

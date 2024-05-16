/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/protocol/kademlia/peer_routing.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "common/main_thread_pool.hpp"
#include "log/logger.hpp"
#include "network/peer_manager.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace kagome {
  /**
   * Kademlia random walk:
   * - Exponential delay.
   * - Don't walk if enough peers are available.
   * https://github.com/paritytech/polkadot-sdk/blob/29c8130bab0ed8216f48e47a78c602e7f0c5c1f2/substrate/client/network/src/discovery.rs#L702-L724
   */
  class KademliaRandomWalk
      : public std::enable_shared_from_this<KademliaRandomWalk> {
    static constexpr std::chrono::seconds kWalkInitialDelay{1};
    static constexpr size_t kExtraPeers{15};

   public:
    KademliaRandomWalk(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        const application::AppConfiguration &app_config,
        common::MainThreadPool &main_thread_pool,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<network::PeerManager> peer_manager,
        std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia)
        : discovery_only_if_under_num_{app_config.outPeers() + kExtraPeers},
          main_pool_handler_{poolHandlerReadyMake(
              this, app_state_manager, main_thread_pool, log_)},
          scheduler_{std::move(scheduler)},
          peer_manager_{std::move(peer_manager)},
          kademlia_{std::move(kademlia)} {}

    bool tryStart() {
      walk();
      return true;
    }

   private:
    void walk() {
      if (peer_manager_->activePeersNumber() < discovery_only_if_under_num_) {
        std::ignore = kademlia_->findRandomPeer();
      }
      auto delay = delay_;
      delay_ = std::min(delay_ * 2, std::chrono::seconds{60});
      auto cb = [weak_self{weak_from_this()}] {
        auto self = weak_self.lock();
        if (not self) {
          return;
        }
        self->walk();
      };
      scheduler_->schedule(std::move(cb), delay);
    }

    log::Logger log_ = log::createLogger("KademliaRandomWalk");
    size_t discovery_only_if_under_num_;
    std::shared_ptr<PoolHandlerReady> main_pool_handler_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<network::PeerManager> peer_manager_;
    std::shared_ptr<libp2p::protocol::kademlia::PeerRouting> kademlia_;

    std::chrono::seconds delay_{kWalkInitialDelay};
  };
}  // namespace kagome

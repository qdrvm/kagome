/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REPUTATIONREPOSITORYIMPL
#define KAGOME_NETWORK_REPUTATIONREPOSITORYIMPL

#include "network/reputation_repository.hpp"

#include <memory>
#include <unordered_map>

#include <libp2p/basic/scheduler.hpp>

namespace kagome::network {

  class ReputationRepositoryImpl
      : public ReputationRepository,
        public std::enable_shared_from_this<ReputationRepositoryImpl> {
   public:
    ReputationRepositoryImpl(
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    Reputation reputation(const PeerId &peer_id) const override;

    Reputation change(const PeerId &peer_id, ReputationChange diff) override;

    Reputation changeForATime(const PeerId &peer_id,
                              ReputationChange diff,
                              std::chrono::seconds duration) override;

   private:
    void ensurePeerPresence(const PeerId &peer_id);

    void tick();

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::unordered_map<PeerId, Reputation> rating_table_;

    libp2p::basic::Scheduler::Handle tick_handler_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REPUTATIONREPOSITORYIMPL

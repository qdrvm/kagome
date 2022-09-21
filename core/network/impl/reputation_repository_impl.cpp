/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/reputation_repository_impl.hpp"

#include <boost/assert.hpp>

using namespace std::chrono_literals;

namespace kagome::network {

  ReputationRepositoryImpl::ReputationRepositoryImpl(
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : scheduler_{std::move(scheduler)} {
    BOOST_ASSERT(scheduler_);

    tick_handler_ = scheduler_->scheduleWithHandle([&] { tick(); }, 1s);
  }

  Reputation ReputationRepositoryImpl::reputation(
      const ReputationRepository::PeerId &peer_id) const {
    return rating_table_.count(peer_id) == 0 ? 0 : rating_table_.at(peer_id);
  }

  Reputation ReputationRepositoryImpl::change(
      const ReputationRepository::PeerId &peer_id, ReputationChange diff) {
    ensurePeerPresence(peer_id);
    return rating_table_[peer_id] += diff.value;
  }

  void ReputationRepositoryImpl::ensurePeerPresence(const PeerId &peer_id) {
    rating_table_.try_emplace(peer_id, 0);
  }

  Reputation ReputationRepositoryImpl::changeForATime(
      const ReputationRepository::PeerId &peer_id,
      ReputationChange diff,
      std::chrono::seconds duration) {
    auto score = change(peer_id, diff);
    scheduler_->schedule(
        [wp{weak_from_this()}, peer_id, diff] {
          if (auto self = wp.lock()) {
            self->change(peer_id, {-diff.value, "Cancel by timeout"});
          }
        },
        duration);
    return score;
  }

  void ReputationRepositoryImpl::tick() {
    // For each elapsed second, move the node reputation towards zero.
    // If we multiply each second the reputation by `k` (where `k` is 0..1), it
    // takes `ln(0.5) / ln(k)` seconds to reduce the reputation by half. Use
    // this formula to empirically determine a value of `k` that looks correct.
    for (auto it = rating_table_.begin(); it != rating_table_.end();) {
      auto cit = it++;
      auto &[peer_id, reputation] = *cit;

      // We use `k = 0.98`, so we divide by `50`. With that value, it takes 34.3
      // seconds to reduce the reputation by half.
      auto diff = reputation / 50;

      if (diff == 0) {
        diff = reputation > 0 ? -1 : 1;
      }
      reputation -= diff;

      if (reputation == 0) {
        rating_table_.erase(it);
      }
    }
    tick_handler_.reschedule(1s);
  }

}  // namespace kagome::network

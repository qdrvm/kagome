/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/reputation_repository_impl.hpp"

#include <boost/assert.hpp>

using namespace std::chrono_literals;

namespace kagome::network {

  ReputationRepositoryImpl::ReputationRepositoryImpl(
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : scheduler_{std::move(scheduler)},
        log_(log::createLogger("Reputation", "reputation")) {
    BOOST_ASSERT(scheduler_);

    tick_handler_ = scheduler_->scheduleWithHandle([&] { tick(); }, 1s);
  }

  Reputation ReputationRepositoryImpl::reputation(
      const ReputationRepository::PeerId &peer_id) const {
    auto it = reputation_table_.find(peer_id);
    return it != reputation_table_.end() ? it->second : 0;
  }

  Reputation ReputationRepositoryImpl::change(
      const ReputationRepository::PeerId &peer_id, ReputationChange diff) {
    auto reputation = reputation_table_[peer_id] += diff.value;
    SL_DEBUG(log_,
             "Reputation of peer {} was changed by {} points to {} points. "
             "Reason: `{}'",
             peer_id,
             diff.value,
             reputation,
             diff.reason);
    return reputation;
  }

  Reputation ReputationRepositoryImpl::changeForATime(
      const ReputationRepository::PeerId &peer_id,
      ReputationChange diff,
      std::chrono::seconds duration) {
    auto reputation = reputation_table_[peer_id] += diff.value;
    SL_DEBUG(log_,
             "Reputation of peer {} was changed by {} points to {} points "
             "for {} seconds. Reason: `{}'",
             peer_id,
             diff.value,
             reputation,
             duration.count(),
             diff.reason);

    auto value = static_cast<Reputation>(
        -static_cast<double>(diff.value)    // opposite original
        * std::pow(0.98, duration.count())  // multiplier considering timeout
    );

    if (value != 0) {
      scheduler_->schedule(
          [wp = weak_from_this(), peer_id, value, reason = diff.reason] {
            if (auto self = wp.lock()) {
              auto reputation = self->reputation_table_[peer_id] += value;
              SL_DEBUG(self->log_,
                       "Reputation of peer {} was changed by {} points to {} "
                       "points. Reason: reverse of `{}'",
                       peer_id,
                       value,
                       reputation,
                       reason);
            }
          },
          duration);
    }

    return reputation;
  }

  void ReputationRepositoryImpl::tick() {
    // For each elapsed second, move the node reputation towards zero.
    // If we multiply each second the reputation by `k` (where `k` is 0..1), it
    // takes `ln(0.5) / ln(k)` seconds to reduce the reputation by half. Use
    // this formula to empirically determine a value of `k` that looks correct.
    for (auto it = reputation_table_.begin(); it != reputation_table_.end();) {
      auto cit = it++;
      auto &peer_id = cit->first;
      auto &reputation = cit->second;

      // We use `k = 0.98`, so we divide by `50`. With that value, it takes 34.3
      // seconds to reduce the reputation by half.
      auto diff = reputation / 50;

      if (diff == 0) {
        diff = reputation < 0 ? -1 : 1;
      }
      reputation -= diff;

      SL_TRACE(
          log_,
          "Reputation of peer {} was changed by {} points to {} points by tick",
          peer_id,
          -diff,
          reputation);

      if (reputation == 0) {
        reputation_table_.erase(cit);
      }
    }
    std::ignore = tick_handler_.reschedule(1s);
  }

}  // namespace kagome::network

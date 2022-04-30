/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/rating_repository_impl.hpp"

#include <boost/assert.hpp>

namespace kagome::network {

  PeerRatingRepositoryImpl::PeerRatingRepositoryImpl(
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : scheduler_{std::move(scheduler)} {
    BOOST_ASSERT(scheduler_);
  }

  PeerScore PeerRatingRepositoryImpl::rating(
      const PeerRatingRepository::PeerId &peer_id) const {
    return rating_table_.count(peer_id) == 0 ? 0 : rating_table_.at(peer_id);
  }

  PeerScore PeerRatingRepositoryImpl::upvote(
      const PeerRatingRepository::PeerId &peer_id) {
    ensurePeerPresence(peer_id);
    return ++rating_table_[peer_id];
  }

  PeerScore PeerRatingRepositoryImpl::downvote(
      const PeerRatingRepository::PeerId &peer_id) {
    ensurePeerPresence(peer_id);
    return --rating_table_[peer_id];
  }

  PeerScore PeerRatingRepositoryImpl::update(
      const PeerRatingRepository::PeerId &peer_id, PeerScore diff) {
    ensurePeerPresence(peer_id);
    return rating_table_[peer_id] += diff;
  }

  void PeerRatingRepositoryImpl::ensurePeerPresence(const PeerId &peer_id) {
    if (rating_table_.count(peer_id) == 0) {
      rating_table_[peer_id] = 0;
    }
  }

  PeerScore PeerRatingRepositoryImpl::upvoteForATime(
      const PeerRatingRepository::PeerId &peer_id,
      std::chrono::seconds duration) {
    auto score = upvote(peer_id);
    scheduler_->schedule(
        [wp{weak_from_this()}, peer_id] {
          if (auto self = wp.lock()) {
            self->downvote(peer_id);
          }
        },
        duration);
    return score;
  }

  PeerScore PeerRatingRepositoryImpl::downvoteForATime(
      const PeerRatingRepository::PeerId &peer_id,
      std::chrono::seconds duration) {
    auto score = downvote(peer_id);
    scheduler_->schedule(
        [wp{weak_from_this()}, peer_id] {
          if (auto self = wp.lock()) {
            self->upvote(peer_id);
          }
        },
        duration);
    return score;
  }

  PeerScore PeerRatingRepositoryImpl::updateForATime(
      const PeerRatingRepository::PeerId &peer_id,
      PeerScore diff,
      std::chrono::seconds duration) {
    auto score = update(peer_id, diff);
    scheduler_->schedule(
        [wp{weak_from_this()}, peer_id, diff] {
          if (auto self = wp.lock()) {
            self->update(peer_id, -1 * diff);
          }
        },
        duration);
    return score;
  }
}  // namespace kagome::network

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RATING_REPOSITORY_IMPL_HPP
#define KAGOME_RATING_REPOSITORY_IMPL_HPP

#include "network/rating_repository.hpp"

#include <memory>
#include <unordered_map>

#include <libp2p/basic/scheduler.hpp>

namespace kagome::network {

  class PeerRatingRepositoryImpl
      : public PeerRatingRepository,
        public std::enable_shared_from_this<PeerRatingRepositoryImpl> {
   public:
    PeerRatingRepositoryImpl(
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    PeerScore rating(const PeerId &peer_id) const override;

    PeerScore upvote(const PeerId &peer_id) override;

    PeerScore downvote(const PeerId &peer_id) override;

    PeerScore update(const PeerId &peer_id, PeerScore diff) override;

    PeerScore upvoteForATime(const PeerId &peer_id,
                             std::chrono::seconds duration) override;

    PeerScore downvoteForATime(const PeerId &peer_id,
                               std::chrono::seconds duration) override;

    PeerScore updateForATime(const PeerId &peer_id,
                             PeerScore diff,
                             std::chrono::seconds duration) override;

   private:
    void ensurePeerPresence(const PeerId &peer_id);

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::unordered_map<PeerId, PeerScore> rating_table_;
  };

}  // namespace kagome::network

#endif  // KAGOME_RATING_REPOSITORY_IMPL_HPP

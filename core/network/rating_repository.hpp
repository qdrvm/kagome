/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RATING_REPOSITORY_HPP
#define KAGOME_RATING_REPOSITORY_HPP

#include <chrono>

#include <libp2p/peer/peer_id.hpp>

namespace kagome::network {

  using PeerScore = std::int32_t;

  /**
   * Storage to handle peers' rating
   */
  class PeerRatingRepository {
   public:
    using PeerId = libp2p::peer::PeerId;

    virtual ~PeerRatingRepository() = default;

    /// Current peer rating
    virtual PeerScore rating(const PeerId &peer_id) const = 0;

    /// Raise peer rating by one, returns resulting rating
    virtual PeerScore upvote(const PeerId &peer_id) = 0;

    /**
     * Raise peer rating by one for a specified amount of time.
     * When time is over, the rating decreases automatically by one.
     * @param peer_id - peer identifier
     * @param duration - amount of time to increase peer rating for
     * @return - resulting peer rating
     */
    virtual PeerScore upvoteForATime(const PeerId &peer_id,
                                     std::chrono::seconds duration) = 0;

    /// Decrease peer rating by one, returns resulting rating
    virtual PeerScore downvote(const PeerId &peer_id) = 0;

    /**
     * Decrease peer rating by one for a specified amount of time.
     * When time is over, the rating increases automatically by one.
     * @param peer_id - peer identifier
     * @param duration - amount of time to decrease peer rating for
     * @return - resulting peer rating
     */
    virtual PeerScore downvoteForATime(const PeerId &peer_id,
                                       std::chrono::seconds duration) = 0;

    /**
     * Change peer rating by arbitrary amount of points
     * @param peer_id - peer identifier
     * @param diff - rating increment or decrement
     * @return - resulting peer rating
     */
    virtual PeerScore update(const PeerId &peer_id, PeerScore diff) = 0;

    /**
     * Change peer rating by arbitrary amount of points for a specified amount
     * of time
     * @param peer_id - peer identifier
     * @param difff - rating increment or decrement
     * @param duration - amount of time to change peer rating for
     * @return - resulting peer rating
     */
    virtual PeerScore updateForATime(const PeerId &peer_id,
                                     PeerScore diff,
                                     std::chrono::seconds duration) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_RATING_REPOSITORY_HPP

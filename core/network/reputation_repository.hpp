/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REPUTATIONREPOSITORY
#define KAGOME_NETWORK_REPUTATIONREPOSITORY

#include <chrono>

#include <libp2p/peer/peer_id.hpp>

#include "network/reputation_change.hpp"

namespace kagome::network {

  using Reputation = std::int32_t;

  /**
   * Storage to handle peers' reputation
   */
  class ReputationRepository {
   public:
    using PeerId = libp2p::peer::PeerId;

    virtual ~ReputationRepository() = default;

    /// Current peer reputation
    virtual Reputation reputation(const PeerId &peer_id) const = 0;

    /**
     * Change peer reputation by arbitrary amount of points
     * @param peer_id - peer identifier
     * @param diff - reputation increment or decrement
     * @return - resulting peer reputation
     */
    virtual Reputation change(const PeerId &peer_id, ReputationChange diff) = 0;

    /**
     * Change peer reputation by arbitrary amount of points for a specified
     * amount of time
     * @param peer_id - peer identifier
     * @param difff - reputation increment or decrement
     * @param duration - amount of time to change peer reputation for
     * @return - resulting peer reputation
     */
    virtual Reputation changeForATime(const PeerId &peer_id,
                                      ReputationChange diff,
                                      std::chrono::seconds duration) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REPUTATIONREPOSITORY

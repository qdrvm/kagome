/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_HPP
#define KAGOME_GOSSIPER_HPP

#include <functional>

#include <libp2p/connection/stream.hpp>
#include <outcome/outcome.hpp>
#include "consensus/grandpa/structs.hpp"
#include "network/types/block_announce.hpp"

namespace kagome::network {
  /**
   * Joins all available gossipers
   */
  struct Gossiper {
    virtual ~Gossiper() = default;
    /**
     * Send BlockAnnounce message
     * @param announce to be sent
     */
    virtual void blockAnnounce(const BlockAnnounce &announce) = 0;

    /**
     * Broadcast grandpa's \param vote_message
     */
    virtual void vote(const consensus::grandpa::VoteMessage &vote_message) = 0;

    /**
     * Broadcast grandpa's \param fin_message
     */
    virtual void finalize(const consensus::grandpa::Fin &fin_message) = 0;

    // Add new stream to gossip
    virtual void addStream(
        std::shared_ptr<libp2p::connection::Stream> stream) = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_HPP

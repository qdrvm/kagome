/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP

#include <libp2p/peer/peer_info.hpp>

#include "consensus/grandpa/structs.hpp"
#include "network/types/grandpa_message.hpp"
#include "outcome/outcome.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class Gossiper
   * @brief Gossip messages to the network via this class
   */
  struct Gossiper {
    virtual ~Gossiper() = default;

    /**
     * Broadcast grandpa's \param vote_message
     */
    virtual void vote(const network::GrandpaVote &vote_message) = 0;

    /**
     * Broadcast grandpa's \param fin_message
     */
    virtual void finalize(const FullCommitMessage &message) = 0;

    /**
     * Send grandpa's \param catch_up_request
     */
    virtual void catchUpRequest(
        const libp2p::peer::PeerId &peer_id,
        const network::CatchUpRequest &catch_up_request) = 0;

    /**
     * Send grandpa's \param catch_up_response
     */
    virtual void catchUpResponse(
        const libp2p::peer::PeerId &peer_id,
        const network::CatchUpResponse &catch_up_response) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP

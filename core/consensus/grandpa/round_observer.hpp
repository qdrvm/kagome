/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

#include <boost/optional.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "consensus/grandpa/structs.hpp"
#include "primitives/justification.hpp"
#include "network/types/grandpa_message.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class RoundObserver
   * @brief observes incoming messages. Abstraction of a network.
   */
  struct RoundObserver {
    virtual ~RoundObserver() = default;

    /**
     * Handler of grandpa vote messages
     * @param peer_id vote owner
     * @param msg vote message
     */
    virtual void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                               const VoteMessage &msg) = 0;

    /**
     * Handler of grandpa finalization messages
     * @param peer_id finalization sender
     * @param f finalization message
     */
    virtual void onFinalize(const libp2p::peer::PeerId &peer_id,
                            const network::FullCommitMessage &f) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "consensus/grandpa/structs.hpp"

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
    virtual void vote(const VoteMessage &vote_message) = 0;

    /**
     * Broadcast grandpa's \param fin_message
     */
    virtual void finalize(const Fin &fin_message) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GOSSIPER_HPP

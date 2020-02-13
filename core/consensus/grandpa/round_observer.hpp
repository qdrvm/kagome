/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class Observer
   * @brief observes incoming messages. Abstraction of a network.
   */
  struct RoundObserver {
    virtual ~RoundObserver() = default;

    /**
     * Handler of grandpa finalization messages
     * @param f finalization message
     */
    virtual void onFin(const Fin &f) = 0;

    /**
     * Handler of grandpa vote messages
     * @param msg vote message
     */
    virtual void onVoteMessage(const VoteMessage &msg) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

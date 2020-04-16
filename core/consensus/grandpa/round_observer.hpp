/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class RoundObserver
   * @brief observes incoming messages. Abstraction of a network.
   */
  struct RoundObserver {
    virtual ~RoundObserver() = default;

    /**
     * Handler of grandpa finalization messages
     * @param f finalization message
     */
    virtual void onFinalize(const Fin &f) = 0;

    /**
     * Handler of grandpa vote messages
     * @param msg vote message
     */
    virtual void onVoteMessage(const VoteMessage &msg) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_OBSERVER_HPP

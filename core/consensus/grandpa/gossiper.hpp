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

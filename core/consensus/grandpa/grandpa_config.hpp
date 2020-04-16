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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  // Structure containing necessary information for running grandpa voting round
  struct GrandpaConfig {
    std::shared_ptr<VoterSet> voters;  // current round's authorities
    RoundNumber round_number;
    Duration
        duration;  // time bound which is enough to gossip messages to everyone
    Id peer_id;    //  key of the peer, do not confuse with libp2p peerid
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP


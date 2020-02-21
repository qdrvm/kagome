/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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


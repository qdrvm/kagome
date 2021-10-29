/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

  /// Structure containing necessary information for running grandpa voting
  /// round
  struct GrandpaConfig {
    /// Current round's authorities
    std::shared_ptr<VoterSet> voters;

    /// Number of round
    RoundNumber round_number;

    /// Time bound which is enough to gossip messages to everyone
    Duration duration;

    /// Key of the peer, do not confuse with libp2p peerid
    std::optional<Id> id;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP

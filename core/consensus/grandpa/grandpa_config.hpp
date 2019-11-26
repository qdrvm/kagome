/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP

#include "consensus/grandpa/voter_set.hpp"

namespace kagome::consensus::grandpa {

struct GrandpaConfig {
  std::shared_ptr<VoterSet> voters;
  RoundNumber round_number;
  Duration duration;
  Id peer_id;
};

}

#endif //KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_CONFIG_HPP

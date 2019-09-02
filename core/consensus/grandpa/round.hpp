/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

#include <boost/optional.hpp>
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/observer.hpp"
#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  struct RoundData {
    Id voter_id;
    Timer prevote_timer;
    Timer precommit_timer;
    std::shared_ptr<Gossiper> outgoing;
  };

  struct RoundState {
    boost::optional<BlockInfo> prevote_ghost;
    boost::optional<BlockInfo> finalized;
    boost::optional<BlockInfo> estimate;
    bool completable;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

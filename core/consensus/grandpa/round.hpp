/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

#include <boost/optional.hpp>
#include "client.hpp"
#include "consensus/grandpa/client.hpp"
#include "consensus/grandpa/observer.hpp"
#include "consensus/grandpa/structs.hpp"
#include "time/timer.hpp"

namespace kagome::consensus::grandpa {

  struct RoundData {
    Id voter_id;
    std::shared_ptr<time::SteadyTimer> prevote_timer;
    std::shared_ptr<time::SteadyTimer> precommit_timer;
    std::shared_ptr<Observer> incoming;
    std::shared_ptr<Client> outgoing;
  };

  struct RoundState {
    boost::optional<BlockInfo> prevote_ghost;
    boost::optional<BlockInfo> finalized;
    boost::optional<BlockInfo> estimate;
    bool completable;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

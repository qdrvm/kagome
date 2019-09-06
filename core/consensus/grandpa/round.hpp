/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

#include <unordered_set>

#include <boost/optional.hpp>
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/observer.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"

namespace kagome::consensus::grandpa {

  struct RoundData {
    Id voter_id;
    Timer prevote_timer;
    Timer precommit_timer;
    std::shared_ptr<Gossiper> outgoing;
  };

  using VotersSet = std::unordered_set<Id>;
  using BitfieldContext = int;  // TODO(warchant): figure out what is that

  struct RoundState {
    // TODO(warchant): complete
   private:
    std::shared_ptr<VoteGraph> graph_;
    std::shared_ptr<VotersSet> voters_;

    VoteTracker<Prevote> prevotes_;
    VoteTracker<Precommit> precommits_;
    HistoricalVotes historical_votes_;
    RoundNumber round_number_;
    BitfieldContext bitfield_{0};
    boost::optional<BlockInfo> prevote_ghost_;
    boost::optional<BlockInfo> precommit_ghost_;
    boost::optional<BlockInfo> finalized_;
    boost::optional<BlockInfo> estimate_;
    bool completable_{false};
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_HPP

//
// Created by taisei on 09.08.2022.
//

#ifndef KAGOME_VOTE_GRAPH_ERROR_HPP
#define KAGOME_VOTE_GRAPH_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum VoteGraphError {
    RECEIVED_BLOCK_LESS_THAN_BASE = 1
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VoteGraphError);

#endif  // KAGOME_VOTE_GRAPH_ERROR_HPP

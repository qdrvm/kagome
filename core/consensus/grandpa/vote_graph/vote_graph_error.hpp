/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VOTE_GRAPH_ERROR_HPP
#define KAGOME_VOTE_GRAPH_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum VoteGraphError { RECEIVED_BLOCK_LESS_THAN_BASE = 1 };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VoteGraphError);

#endif  // KAGOME_VOTE_GRAPH_ERROR_HPP

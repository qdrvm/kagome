/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/vote_graph/vote_graph_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, VoteGraphError, e) {
  using E = kagome::consensus::grandpa::VoteGraphError;
  switch (e) {
    case E::RECEIVED_BLOCK_LESS_THAN_BASE:
      return "Received block has smaller height than base in graph";
  }
  return "Unknown error (invalid VoteGraphError)";
}

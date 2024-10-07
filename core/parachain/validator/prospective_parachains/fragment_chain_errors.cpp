/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain_errors.hpp"
#include "utils/stringify.hpp"

#define COMPONENT FragmentChain
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            FragmentChainError,
                            e) {
  using E = decltype(e);
  switch (e) {
    case E::CANDIDATE_ALREADY_KNOWN:
      return COMPONENT_NAME ": Candidate already known";
    case E::INTRODUCE_BACKED_CANDIDATE:
      return COMPONENT_NAME ": Introduce backed candidate";
    case E::CYCLE:
      return COMPONENT_NAME ": Cycle";
    case E::MULTIPLE_PATH:
      return COMPONENT_NAME ": Multiple path";
    case E::ZERO_LENGTH_CYCLE:
      return COMPONENT_NAME ": Zero length cycle";
    case E::RELAY_PARENT_NOT_IN_SCOPE:
      return COMPONENT_NAME ": Relay parent not in scope";
    case E::RELAY_PARENT_PRECEDES_CANDIDATE_PENDING_AVAILABILITY:
      return COMPONENT_NAME
          ": Relay parent precedes candidate pending availability";
    case E::FORK_WITH_CANDIDATE_PENDING_AVAILABILITY:
      return COMPONENT_NAME ": Fork with candidate pending availability";
    case E::FORK_CHOICE_RULE:
      return COMPONENT_NAME ": Fork choice rule";
    case E::PARENT_CANDIDATE_NOT_FOUND:
      return COMPONENT_NAME ": Parent candidate not found";
    case E::COMPUTE_CONSTRAINTS:
      return COMPONENT_NAME ": Compute constraints";
    case E::CHECK_AGAINST_CONSTRAINTS:
      return COMPONENT_NAME ": Check against constraints";
    case E::RELAY_PARENT_MOVED_BACKWARDS:
      return COMPONENT_NAME ": Relay parent moved backwards";
  }
  return COMPONENT_NAME ": unknown error";
}

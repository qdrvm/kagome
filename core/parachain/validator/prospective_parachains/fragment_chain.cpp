/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include "utils/stringify.hpp"

#define COMPONENT FragmentChain
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment,
                            FragmentChain::Error,
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

namespace kagome::parachain::fragment {

  outcome::result<void> FragmentChain::check_not_contains_candidate(
      const CandidateHash &candidate_hash) const {
    if (best_chain.contains(candidate_hash)
        || unconnected.contains(candidate_hash)) {
      return Error::CANDIDATE_ALREADY_KNOWN;
    }
    return outcome::success();
  }

  outcome::result<void> FragmentChain::check_cycles_or_invalid_tree(
      const Hash &output_head_hash) const {
    if (best_chain.by_parent_head.contains(output_head_hash)) {
      return Error::CYCLE;
    }

    if (best_chain.by_output_head.contains(output_head_hash)) {
      return Error::MULTIPLE_PATH;
    }

    return outcome::success();
  }

  outcome::result<void> FragmentChain::try_adding_seconded_candidate(
      const CandidateEntry &candidate) {
    if (candidate.state == CandidateState::Backed) {
      return Error::INTRODUCE_BACKED_CANDIDATE;
    }

    OUTCOME_TRY(can_add_candidate_as_potential(candidate));
    return unconnected.add_candidate_entry(candidate);
  }

}  // namespace kagome::parachain::fragment

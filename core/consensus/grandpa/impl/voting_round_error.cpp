/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, VotingRoundError, e) {
  using E = kagome::consensus::grandpa::VotingRoundError;
  switch (e) {
    case E::INVALID_SIGNATURE:
      return "Invalid signature";
    case E::REDUNDANT_EQUIVOCATION:
      return "Redundant equivocation";
    case E::NOT_ENOUGH_WEIGHT:
      return "Non enough weigh of votes";
    case E::JUSTIFICATION_FOR_ROUND_IN_PAST:
      return "Justification for passed round";
    case E::JUSTIFICATION_FOR_BLOCK_IN_PAST:
      return "Justification for early finalized block";
    case E::LAST_ESTIMATE_BETTER_THAN_PREVOTE:
      return "Current state does not contain prevote which is equal to the "
             "last round estimate or is descendant of it";
    case E::UNKNOWN_VOTER:
      return "Provided vote is the vote of unknown voter";
    case E::ZERO_WEIGHT_VOTER:
      return "Provided vote of zero weight voter";
    case E::DUPLICATED_VOTE:
      return "Provided vote duplicates the existing one";
    case E::EQUIVOCATED_VOTE:
      return "Provided vote equivocated the existing one";
    case E::NO_PREVOTE_CANDIDATE:
      return "Can't get best prevote candidate";
  }
  return "Unknown error (invalid VotingRoundError)";
}

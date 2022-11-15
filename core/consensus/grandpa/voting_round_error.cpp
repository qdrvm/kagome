/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "voting_round_error.hpp"

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
    case E::JUSTIFICATION_FOR_AUTHORITY_SET_IN_PAST:
      return "Justification for an obsolete authority set";
    case E::LAST_ESTIMATE_BETTER_THAN_PREVOTE:
      return "Current state does not contain prevote which is equal to the "
             "last round estimate or is descendant of it";
    case E::JUSTIFIED_BLOCK_IS_GREATER_THAN_ACTUALLY_FINALIZED:
      return "Justified block is greater than actually finalized";
    case E::NO_KNOWN_AUTHORITIES_FOR_BLOCK:
      return "Can't retrieve authorities for the given block; Probably block "
             "is not retrieved yet";
    case E::VOTER_SET_NOT_FOUND_FOR_BLOCK:
      return "Voter set not found for block; Probably block already finalized "
             "but is not last finalized";
    case E::WRONG_ORDER_OF_VOTER_SET_ID:
      return "New round has an abnormal voter set id, expected "
             "equal or greater by one than the previous voter set id";
    case E::UNKNOWN_VOTER:
      return "Provided vote's voter is unknown";
    case E::ZERO_WEIGHT_VOTER:
      return "Provided vote of disabled (zero weight) voter";
    case E::DUPLICATED_VOTE:
      return "Provided vote duplicates the existing one";
    case E::EQUIVOCATED_VOTE:
      return "Provided vote equivocated the existing one";
    case E::VOTE_OF_KNOWN_EQUIVOCATOR:
      return "Provided vote is vote of known equivocator";
    case E::NO_PREVOTE_CANDIDATE:
      return "Can't get best prevote candidate";
    case E::ROUND_IS_NOT_FINALIZABLE:
      return "Round is not finalizable";
  }
  return "Unknown error (invalid VotingRoundError)";
}

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa, VotingRoundError, e) {
  using E = kagome::consensus::grandpa::VotingRoundError;
  switch (e) {
    case E::FIN_VALIDATION_FAILED:
      return "Validation of finalization message failed";
    case E::LAST_ESTIMATE_BETTER_THAN_PREVOTE:
      return "Current state does not contain prevote which is equal to the "
             "last round estimate or is descendant of it";
	  case E::UNKNOWN_VOTER:
		  return "Provided vote is the vote of unknown voter";
	  case E::DUPLICATED_VOTE:
		  return "Provided vote duplicates the existing one";
	  case E::EQUIVOCATED_VOTE:
		  return "Provided vote equivocated the existing one";
	  case E::NO_PREVOTE_CANDIDATE:
		  return "Can't get best prevote candidate";
  }
}

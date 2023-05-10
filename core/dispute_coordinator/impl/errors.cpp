/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, RollingSessionWindowError, e) {
  using E = kagome::dispute::RollingSessionWindowError;
  switch (e) {
    case E::SessionsUnavailable:
      return "Session unavailable";
  }
  return "unknown error (invalid RollingSessionWindowError)";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, SignatureValidationError, e) {
  using E = kagome::dispute::SignatureValidationError;
  switch (e) {
    case E::InvalidSignature:
      return "Invalid argument occurred";
    case E::MissingPublicKey:
      return "Missing public key for validator";
  }
  return "unknown error (invalid SignatureValidationError)";
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, DisputeMessageCreationError, e) {
  using E = kagome::dispute::DisputeMessageCreationError;
  switch (e) {
    case E::NoOppositeVote:
      return "There was no opposite vote available";
    case E::InvalidValidatorIndex:
      return "Found vote had an invalid validator index that couldn't be found";
    case E::InvalidStoredStatement:
      return "Statement found in votes had invalid signature.";
    case E::InvalidStatementCombination:
      return "Invalid statement combination";
  }
  return "unknown error (invalid DisputeMessageCreationError)";
};

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute,
                            DisputeMessageConstructingError,
                            e) {
  using E = kagome::dispute::DisputeMessageConstructingError;
  switch (e) {
    case E::CandidateHashMismatch:
      return "Candidate hashes of the two votes did not match up";
    case E::SessionIndexMismatch:
      return "Session indices of the two votes did not match up";
    case E::InvalidValidKey:
      return "Valid statement validator key did not match session information";
    case E::InvalidInvalidKey:
      return "Invalid statement validator key did not match session "
             "information";
    case E::InvalidCandidateReceipt:
      return "Hash of candidate receipt did not match provided hash";
    case E::ValidStatementHasInvalidKind:
      return "Valid statement has kind `invalid`";
    case E::InvalidStatementHasValidKind:
      return "Invalid statement has kind `valid`";
    case E::ValidStatementInvalidValidatorIndex:
      return "The valid statement had an invalid validator index";
    case E::InvalidStatementInvalidValidatorIndex:
      return "The invalid statement had an invalid validator index";
  }
  return "unknown error (invalid DisputeMessageConstructingError)";
};

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, SessionObtainingError, e) {
  using E = kagome::dispute::SessionObtainingError;
  switch (e) {
    case E::SessionsUnavailable:
      return "Session unavailable";
    case E::RuntimeApiError:
      return "Error while fetching session information";
    case E::Missing:
      return "Session info missing from runtime";
    case E::NoSuchSession:
      return "There was no session with the given index";
  }
  return "unknown error (invalid SessionObtainingError)";
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

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, DisputeProcessingError, e) {
  using E = kagome::dispute::DisputeProcessingError;
  switch (e) {
    case E::NotAValidator:
      return "Peer attempted to participate in dispute and is not a validator";
    case E::AuthorityFlooding:
      return "Authority sent messages at a too high rate";
  }
  return "unknown error (invalid DisputeProcessingError)";
};

OUTCOME_CPP_DEFINE_CATEGORY(kagome::dispute, BatchError, e) {
  using E = kagome::dispute::BatchError;
  switch (e) {
    case E::MaxBatchLimitReached:
      return "Had to drop messages, because we reached limit on concurrent "
             "batches";
    case E::RedundantMessage:
      return "Received votes from peer have been completely redundant";
  }
  return "unknown error (invalid BatchError)";
};

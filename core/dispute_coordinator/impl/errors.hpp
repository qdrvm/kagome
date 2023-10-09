/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::dispute {

  enum class SessionObtainingError {
    SessionsUnavailable = 1,
    RuntimeApiError,
    Missing,
    /// We tried fetching a session info which was not available.
    NoSuchSession,
  };

  enum SignatureValidationError {
    /// Invalid signature
    InvalidSignature = 1,

    /// Missing public key for validator,
    MissingPublicKey,
  };

  enum class DisputeMessageCreationError {
    /// There was no opposite vote available
    NoOppositeVote = 1,

    /// Found vote had an invalid validator index that could not be found
    InvalidValidatorIndex,

    /// Statement found in votes had invalid signature
    InvalidStoredStatement,

    /// Invalid statement combination
    InvalidStatementCombination,
  };

  /// Things that can go wrong when constructing a `DisputeMessage`.
  enum class DisputeMessageConstructingError {
    /// The statements concerned different candidates.
    CandidateHashMismatch = 1,

    /// The statements concerned different sessions.
    SessionIndexMismatch,

    /// The valid statement validator key did not correspond to passed in
    /// `SessionInfo`.
    InvalidValidKey,

    /// The invalid statement validator key did not correspond to passed in
    /// `SessionInfo`.
    InvalidInvalidKey,

    /// Provided receipt had different hash than the `CandidateHash` in the
    /// signed statements.
    InvalidCandidateReceipt,

    /// Valid statement should have `ValidDisputeStatement`.
    ValidStatementHasInvalidKind,

    /// Invalid statement should have `InvalidDisputeStatement`.
    InvalidStatementHasValidKind,

    /// Provided index could not be found in `SessionInfo`.
    ValidStatementInvalidValidatorIndex,

    /// Provided index could not be found in `SessionInfo`.
    InvalidStatementInvalidValidatorIndex,
  };

  enum class DisputeProcessingError {
    /// Peer attempted to participate in dispute and is not a validator
    NotAValidator = 1,
    /// Authority sent messages at a too high rate
    AuthorityFlooding,
  };

  enum class BatchError {
    /// Had to drop messages, because we reached limit on concurrent batches
    MaxBatchLimitReached = 1,
    // Received votes from peer have been completely redundant
    RedundantMessage,
  };

}  // namespace kagome::dispute

OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, SessionObtainingError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, SignatureValidationError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, DisputeMessageCreationError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, DisputeMessageConstructingError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, DisputeProcessingError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, BatchError);

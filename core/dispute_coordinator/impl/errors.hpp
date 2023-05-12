/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_ERRORS_HPP
#define KAGOME_DISPUTE_ERRORS_HPP

#include "outcome/outcome.hpp"

namespace kagome::dispute {

  enum class RollingSessionWindowError {
    SessionsUnavailable = 1,
    RuntimeApiError,
    Missing,
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

    /// Valid statement should have `ValidDisputeStatementKind`.
    ValidStatementHasInvalidKind,

    /// Invalid statement should have `InvalidDisputeStatementKind`.
    InvalidStatementHasValidKind,

    /// Provided index could not be found in `SessionInfo`.
    ValidStatementInvalidValidatorIndex,

    /// Provided index could not be found in `SessionInfo`.
    InvalidStatementInvalidValidatorIndex,
  };

}  // namespace kagome::dispute

OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, RollingSessionWindowError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, SignatureValidationError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, DisputeMessageCreationError);
OUTCOME_HPP_DECLARE_ERROR(kagome::dispute, DisputeMessageConstructingError);

#endif  // KAGOME_DISPUTE_ERRORS_HPP

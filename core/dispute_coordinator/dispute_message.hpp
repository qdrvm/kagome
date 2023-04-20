/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTEMESSAGE
#define KAGOME_DISPUTE_DISPUTEMESSAGE

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  /// Any invalid vote (currently only explicit).
  struct InvalidDisputeVote {
    /// The voting validator index.
    ValidatorIndex index;

    /// The validator signature, that can be verified when constructing a
    /// `SignedDisputeStatement`.
    ValidatorSignature signature;

    /// Kind of dispute statement.
    InvalidDisputeStatementKind kind;
  };

  /// Any valid vote (backing, approval, explicit).
  struct ValidDisputeVote {
    /// The voting validator index.
    ValidatorIndex index;

    /// The validator signature, that can be verified when constructing a
    /// `SignedDisputeStatement`.
    ValidatorSignature signature;

    /// Kind of dispute statement.
    ValidDisputeStatementKind kind;
  };

  /// A dispute initiating/participating message that have been built from
  /// signed
  /// statements.
  ///
  /// And most likely has been constructed correctly. This is used with
  /// `DisputeDistributionMessage::SendDispute` for sending out votes.
  ///
  /// NOTE: This is sent over the wire, any changes are a change in protocol and
  /// need to be versioned.
  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/primitives/src/disputes/message.rs#L40
  struct DisputeMessage {
    /// The candidate being disputed.
    CandidateReceipt candidate_receipt;

    /// The session the candidate appears in.
    SessionIndex session_index;

    /// The invalid vote data that makes up this dispute.
    InvalidDisputeVote invalid_vote;

    /// The valid vote that makes this dispute request valid.
    ValidDisputeVote valid_vote;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTEMESSAGE

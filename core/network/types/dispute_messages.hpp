/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DISPUTEMESSAGE
#define KAGOME_NETWORK_DISPUTEMESSAGE

#include "dispute_coordinator/types.hpp"
#include "scale/tie.hpp"

namespace kagome::network {

  using InvalidDisputeStatement = dispute::InvalidDisputeStatement;
  using ValidDisputeStatement = dispute::ValidDisputeStatement;

  /// Any invalid vote (currently only explicit).
  struct InvalidDisputeVote {
    SCALE_TIE(3);

    /// The voting validator index.
    ValidatorIndex index;

    /// The validator signature, that can be verified when constructing a
    /// `SignedDisputeStatement`.
    ValidatorSignature signature;

    /// Kind of dispute statement.
    InvalidDisputeStatement kind;
  };

  /// Any valid vote (backing, approval, explicit).
  struct ValidDisputeVote {
    SCALE_TIE(3);

    /// The voting validator index.
    ValidatorIndex index;

    /// The validator signature, that can be verified when constructing a
    /// `SignedDisputeStatement`.
    ValidatorSignature signature;

    /// Kind of dispute statement.
    ValidDisputeStatement kind;
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
    SCALE_TIE(4);

    /// The candidate being disputed.
    CandidateReceipt candidate_receipt;

    /// The session the candidate appears in.
    SessionIndex session_index;

    /// The invalid vote data that makes up this dispute.
    InvalidDisputeVote invalid_vote;

    /// The valid vote that makes this dispute request valid.
    ValidDisputeVote valid_vote;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_DISPUTEMESSAGE

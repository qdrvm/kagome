/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_TYPES_HPP
#define KAGOME_DISPUTE_TYPES_HPP

#include "dispute_coordinator/dispute_coordinator.hpp"

#include <unordered_set>

#include "parachain/types.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::dispute {

  using parachain::ValidatorId;
  using runtime::SessionInfo;

  struct StoredWindow {
    SCALE_TIE(2);

    SessionIndex earliest_session;
    std::vector<SessionInfo> session_info;
  };

  /// An explicit statement issued as part of a dispute.
  using Explicit = Tagged<Empty, struct ExplicitTag>;
  /// A seconded statement on a candidate from the backing phase.
  using BackingSeconded = Tagged<CandidateHash, struct BackingSecondedTag>;
  /// A valid statement on a candidate from the backing phase.
  using BackingValid = Tagged<CandidateHash, struct BackingValidTag>;
  /// An approval vote from the approval checking phase.
  using ApprovalChecking = Tagged<Empty, struct ApprovalCheckingTag>;

  /// Different kinds of statements of validity on a candidate.
  using ValidDisputeStatementKind =
      boost::variant<Explicit, BackingSeconded, BackingValid, ApprovalChecking>;

  /// Different kinds of statements of invalidity on a candidate.
  using InvalidDisputeStatementKind = boost::variant<Explicit>;

  struct ValidDisputeStatement {
    ValidDisputeStatementKind kind;
    ValidatorIndex index;
    ValidatorSignature signature;
  };

  struct InvalidDisputeStatement {
    InvalidDisputeStatementKind kind;
    ValidatorIndex index;
    ValidatorSignature signature;
  };

  /// A statement about a candidate, to be used within some dispute resolution
  /// process.
  ///
  /// Statements are either in favor of the candidate's validity or against it.
  using DisputeStatement_ = boost::variant<
      /// A valid statement, of the given kind
      ValidDisputeStatement,  // Valid
      /// An invalid statement, of the given kind.
      InvalidDisputeStatement>;  // Invalid

  /// Tracked votes on candidates, for the purposes of dispute resolution.
  struct CandidateVotes {
    SCALE_TIE(3);

    /// The receipt of the candidate itself.
    CandidateReceipt candidate_receipt;
    /// Votes of validity, sorted by validator index.
    std::map<ValidatorIndex, ValidDisputeStatement> valid;
    /// Votes of invalidity, sorted by validator index.
    std::map<ValidatorIndex, InvalidDisputeStatement> invalid;
  };

  /// Timestamp based on the 1 Jan 1970 UNIX base, which is persistent across
  /// node restarts and OS reboots.
  using Timestamp = uint64_t;

  /// The status of dispute.
  /// NOTE: This status is persisted to the database
  using DisputeStatus = boost::variant<
      /// The dispute is active and unconcluded.
      Tagged<Empty, struct Active>,

      /// The dispute has been concluded in favor of the candidate
      /// since the given timestamp.
      Tagged<Timestamp, struct ConcludedFor>,
      /// The dispute has been concluded against the candidate
      /// since the given timestamp.
      ///
      /// This takes precedence over `ConcludedFor` in the case that
      /// both are true, which is impossible unless a large amount of
      /// validators are participating on both sides.
      Tagged<Timestamp, struct ConcludedAgainst>,

      /// Dispute has been confirmed (more than `byzantine_threshold` have
      /// already participated/ or we have seen the candidate included
      /// already/participated successfully ourselves).
      Tagged<Empty, struct Confirmed>>;

  /// The mapping for recent disputes; any which have not yet been pruned for
  /// being ancient.
  using RecentDisputes =
      std::map<std::tuple<SessionIndex, CandidateHash>, DisputeStatus>;

  using MaybeCandidateReceipt = boost::variant<
      /// Directly provides the candidate receipt.
      CandidateReceipt,  // Provides
      /// Assumes it was seen before by means of seconded message.
      CandidateHash>;  // AssumeBackingVotePresent

  /// A checked dispute statement from an associated validator.
  struct SignedDisputeStatement {
    DisputeStatement_ dispute_statement;
    CandidateHash candidate_hash;
    ValidatorId validator_public;
    ValidatorSignature validator_signature;
    SessionIndex session_index;
  };

  using Voted = std::vector<
      std::tuple<ValidatorIndex, DisputeStatement, ValidatorSignature>>;

  struct CannotVote : public Empty {};

  /// Whether or not we already issued some statement about a candidate.
  using OwnVoteState = boost::variant<
      /// Our votes, if any.
      Voted,

      /// We are not a parachain validator in the session.
      ///
      /// Hence we cannot vote.
      CannotVote>;

  struct CandidateEnvironment {
    /// The session the candidate appeared in.
    SessionIndex session_index;
    /// Session for above index.
    SessionInfo &session;
    /// Validator indices controlled by this node.
    std::unordered_set<ValidatorIndex> controlled_indices;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_TYPES_HPP

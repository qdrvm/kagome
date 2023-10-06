/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_TYPES_HPP
#define KAGOME_DISPUTE_TYPES_HPP

#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>

#include "common/visitor.hpp"
#include "parachain/types.hpp"
#include "primitives/block.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::dispute {

  using network::CandidateReceipt;
  using parachain::CandidateHash;
  using parachain::GroupIndex;
  using parachain::SessionIndex;
  using parachain::ValidatorId;
  using parachain::ValidatorIndex;
  using parachain::ValidatorSignature;
  using runtime::SessionInfo;
  template <typename T>
  using Indexed = parachain::Indexed<T>;

  template <typename Result>
  using CbOutcome = std::function<void(outcome::result<Result>)>;

  struct StoredWindow {
    SCALE_TIE(2);

    SessionIndex earliest_session;
    std::vector<SessionInfo> session_info;
  };

  // Different kinds of statements of validity/invalidity on a candidate.

  /// An explicit statement issued as part of a dispute.
  using Explicit = Tagged<Empty, struct ExplicitTag>;
  /// A seconded statement on a candidate from the backing phase.
  using BackingSeconded = Tagged<CandidateHash, struct BackingSecondedTag>;
  /// A valid statement on a candidate from the backing phase.
  using BackingValid = Tagged<CandidateHash, struct BackingValidTag>;
  /// An approval vote from the approval checking phase.
  using ApprovalChecking = Tagged<Empty, struct ApprovalCheckingTag>;

  /// A valid statement, of the given kind
  using ValidDisputeStatement =
      boost::variant<Explicit, BackingSeconded, BackingValid, ApprovalChecking>;

  /// An invalid statement, of the given kind.
  using InvalidDisputeStatement = boost::variant<Explicit>;

  /// A statement about a candidate, to be used within some dispute resolution
  /// process.
  ///
  /// Statements are either in favor of the candidate's validity or against it.
  using DisputeStatement = boost::variant<ValidDisputeStatement,   // 0
                                          InvalidDisputeStatement  // 1
                                          >;

  /// Tracked votes on candidates, for the purposes of dispute resolution.
  struct CandidateVotes {
    SCALE_TIE(3);

    /// The receipt of the candidate itself.
    CandidateReceipt candidate_receipt;
    /// Votes of validity, sorted by validator index.
    std::map<ValidatorIndex,
             std::tuple<ValidDisputeStatement, ValidatorSignature>>
        valid{};
    /// Votes of invalidity, sorted by validator index.
    std::map<ValidatorIndex,
             std::tuple<InvalidDisputeStatement, ValidatorSignature>>
        invalid{};
  };

  /// Timestamp based on the 1 Jan 1970 UNIX base, which is persistent across
  /// node restarts and OS reboots.
  using Timestamp = uint64_t;

  /// The dispute is active and unconcluded.
  using Active = Tagged<Empty, struct ActiveTag>;

  /// The dispute has been concluded in favor of the candidate since the given
  /// timestamp.
  using ConcludedFor = Tagged<Timestamp, struct ConcludedForTag>;

  /// The dispute has been concluded against the candidate since the given
  /// timestamp.
  ///
  /// This takes precedence over `ConcludedFor` in the case that both are true,
  /// which is impossible unless a large amount of validators are participating
  /// on both sides.
  using ConcludedAgainst = Tagged<Timestamp, struct ConcludedAgainstTag>;

  /// Dispute has been confirmed (more than `byzantine_threshold` have already
  /// participated/ or we have seen the candidate included already/participated
  /// successfully ourselves).
  using Confirmed = Tagged<Empty, struct ConfirmedTag>;

  /// The status of dispute.
  /// NOTE: This status is persisted to the database
  using DisputeStatus =
      boost::variant<Active, ConcludedFor, ConcludedAgainst, Confirmed>;

  /// The mapping for recent disputes; any which have not
  /// yet been pruned for being ancient.
  using RecentDisputes =
      std::map<std::tuple<SessionIndex, CandidateHash>, DisputeStatus>;

  using MaybeCandidateReceipt = boost::variant<
      /// Directly provides the candidate receipt.
      CandidateReceipt,  // Provides
      /// Assumes it was seen before by means of seconded message.
      CandidateHash>;  // AssumeBackingVotePresent

  /// A checked dispute statement from an associated validator.
  struct SignedDisputeStatement {
    DisputeStatement dispute_statement;
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
    std::unordered_set<ValidatorIndex> controlled_indices{};
  };

  /// The status of an activated leaf.
  enum class LeafStatus : bool {
    /// A leaf is fresh when it's the first time the leaf has been encountered.
    /// Most leaves should be fresh.
    Fresh,

    /// A leaf is stale when it's encountered for a subsequent time. This will
    /// happen
    /// when the chain is reverted or the fork-choice rule abandons some chain.
    Stale,
  };

  /// Activated leaf.
  struct ActivatedLeaf {
    /// The block hash.
    primitives::BlockHash hash;

    /// The block number.
    primitives::BlockNumber number;

    /// The status of the leaf.
    LeafStatus status;

    /// An associated [`jaeger::Span`].
    ///
    /// NOTE: Each span should only be kept active as long as the leaf is
    /// considered active and should be dropped when the leaf is deactivated.
    // Arc<jaeger::Span> span;
  };

  /// Changes in the set of active leaves: the parachain heads which we care to
  /// work on.
  ///
  /// Note that the activated and deactivated fields indicate deltas, not
  /// complete sets.
  struct ActiveLeavesUpdate {
    /// New relay chain block of interest.
    std::optional<ActivatedLeaf> activated;

    /// Relay chain block hashes no longer of interest.
    std::vector<primitives::BlockHash> deactivated{};
  };

  /// Implicit validity attestation by issuing.
  /// This corresponds to issuance of a `Candidate` statement.
  using ImplicitValidityAttestation =
      Tagged<ValidatorSignature, struct ImplicitTag>;

  /// An explicit attestation. This corresponds to issuance of a
  /// `Valid` statement.
  using ExplicitValidityAttestation =
      Tagged<ValidatorSignature, struct ExplicitTag>;

  /// An either implicit or explicit attestation to the validity of a parachain
  /// candidate.
  /// Note: order of types in variant matters
  using ValidityAttestation = boost::variant<Unused<0>,
                                             ImplicitValidityAttestation,   // 1
                                             ExplicitValidityAttestation>;  // 2

  /// A set of statements about a specific candidate.
  struct DisputeStatementSet {
    SCALE_TIE(3);

    /// The candidate referenced by this set.
    CandidateHash candidate_hash;

    /// The session index of the candidate.
    SessionIndex session;

    /// Statements about the candidate.
    std::vector<
        std::tuple<DisputeStatement, ValidatorIndex, ValidatorSignature>>
        statements;
  };

  /// A set of dispute statements.
  using MultiDisputeStatementSet = std::vector<DisputeStatementSet>;

  /// Scraped runtime backing votes and resolved disputes.
  struct ScrapedOnChainVotes {
    SCALE_TIE(3);

    /// The session in which the block was included.
    SessionIndex session;

    /// Set of backing validators for each candidate, represented by its
    /// candidate receipt.
    std::vector<
        std::pair<CandidateReceipt,
                  std::vector<std::pair<ValidatorIndex, ValidityAttestation>>>>
        backing_validators_per_candidate;

    /// On-chain-recorded set of disputes.
    /// Note that the above `backing_validators` are
    /// unrelated to the backers of the disputes candidates.
    MultiDisputeStatementSet disputes;
  };

  /// Describes a relay-chain block by the para-chain candidates
  /// it includes.
  struct BlockDescription {
    /// The relay-chain block hash.
    primitives::BlockHash block_hash;
    /// The session index of this block.
    SessionIndex session;
    /// The set of para-chain candidates.
    std::vector<CandidateHash> candidates;
  };

  /// Determine the highest undisputed block within the given chain, based on
  /// where candidates were included. If even the base block should not be
  /// finalized due to a dispute, then `None` should be returned on the channel.
  ///
  /// The block descriptions begin counting upwards from the block after the
  /// given `base_number`. The `base_number` is typically the number of the last
  /// finalized block but may be slightly higher. This block is inevitably going
  /// to be finalized so it is not accounted for by this function.
  struct DetermineUndisputedChain {
    /// The lowest possible block to vote on.
    primitives::BlockInfo base;
    /// Descriptions of all the blocks counting upwards from the block after the
    /// base number
    std::vector<BlockDescription> block_descriptions;
    /// The block to vote on, might be base in case there is no better.
    // tx: oneshot::Sender<(BlockNumber, Hash)>,
  };

  /// ScrapedUpdates
  ///
  /// Updates to on_chain_votes and included receipts for new active leaf and
  /// its unprocessed ancestors.
  ///
  /// on_chain_votes: New votes as seen on chain
  /// included_receipts: Newly included parachain block candidate receipts as
  /// seen on chain
  struct ScrapedUpdates {
    std::vector<ScrapedOnChainVotes> on_chain_votes;
    std::vector<CandidateReceipt> included_receipts;
  };

  /// Ready for import.
  struct PreparedImport {
    CandidateReceipt candidate_receipt;
    std::vector<Indexed<SignedDisputeStatement>> statements{};
    /// Information about original requesters.
    std::vector<std::tuple<libp2p::peer::PeerId,
                           std::function<void(outcome::result<void>)>>>
        requesters;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_TYPES_HPP

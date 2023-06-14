/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_DISPUTECOORDINATOR
#define KAGOME_DISPUTE_DISPUTECOORDINATOR

#include "network/dispute_request_observer.hpp"

#include "dispute_coordinator/types.hpp"

namespace kagome::dispute {

  class DisputeCoordinator {
   public:
    using QueryCandidateVotes =
        std::vector<std::pair<SessionIndex, CandidateHash>>;
    using OutputCandidateVotes =
        std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>>;
    using OutputDisputes =
        std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>;

    virtual ~DisputeCoordinator() = default;

    /// Import statements by validators about a candidate.
    ///
    /// The subsystem will silently discard ancient statements or sets of only
    /// dispute-specific statements for candidates that are previously unknown
    /// to the subsystem. The former is simply because ancient data is not
    /// relevant and the latter is as a DoS prevention mechanism. Both backing
    /// and approval statements already undergo anti-DoS procedures in their
    /// respective subsystems, but statements cast specifically for disputes are
    /// not necessarily relevant to any candidate the system is already aware of
    /// and thus present a DoS vector. Our expectation is that nodes will notify
    /// each other of disputes over the network by providing (at least) 2
    /// conflicting statements, of which one is either a backing or validation
    /// statement.
    ///
    /// This does not do any checking of the message signature.
    ///
    /// @param candidate_receipt - The candidate receipt itself
    /// @param session - The session the candidate appears in
    /// @param statements - Statements, with signatures checked, by validators
    /// participating in disputes. The validator index passed alongside each
    /// statement should correspond to the index of the validator in the set.
    /// @param cb - Callback for result
    virtual void handle_incoming_ImportStatements(
        CandidateReceipt candidate_receipt,
        SessionIndex session,
        std::vector<Indexed<SignedDisputeStatement>>
            statements,  // FIXME avoid copy
        CbOutcome<void> &&cb) = 0;

    /// Fetch a list of all recent disputes the coordinator is aware of.
    /// These are disputes which have occurred any time in recent sessions,
    /// and which may have already concluded.
    virtual void handle_incoming_RecentDisputes(
        CbOutcome<OutputDisputes> &&cb) = 0;

    /// Fetch a list of all active disputes that the coordinator is aware of.
    /// These disputes are either not yet concluded or recently concluded.
    virtual void handle_incoming_ActiveDisputes(
        CbOutcome<OutputDisputes> &&cb) = 0;

    /// Get candidate votes for a candidate.
    virtual void handle_incoming_QueryCandidateVotes(
        const QueryCandidateVotes &msg,
        CbOutcome<OutputCandidateVotes> &&cb) = 0;

    /// Sign and issue local dispute votes. A value of `true` indicates
    /// validity, and `false` invalidity.
    virtual void handle_incoming_IssueLocalStatement(
        SessionIndex session,
        CandidateHash candidate_hash,
        CandidateReceipt candidate_receipt,
        bool valid,
        CbOutcome<void> &&cb) = 0;

    /// Determine the highest undisputed block within the given chain, based
    /// on where candidates were included. If even the base block should not
    /// be finalized due to a dispute, then `None` should be returned on the
    /// channel.
    ///
    /// The block descriptions begin counting upwards from the block after the
    /// given `base_number`. The `base_number` is typically the number of the
    /// last finalized block but may be slightly higher. This block is
    /// inevitably going to be finalized so it is not accounted for by this
    /// function.
    ///
    /// @param base - The lowest possible block to vote on
    /// @param block_descriptions - Descriptions of all the blocks counting
    /// upwards from the block after the base number
    /// @param cb - Callback for result
    /// @note The block to vote on, might be base in case there is no better.
    virtual void handle_incoming_DetermineUndisputedChain(
        primitives::BlockInfo base,
        std::vector<BlockDescription> block_descriptions,
        CbOutcome<primitives::BlockInfo> &&cb) = 0;

    virtual void getDisputeForInherentData(
        const primitives::BlockInfo &relay_parent,
        std::function<void(MultiDisputeStatementSet)> &&cb) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_DISPUTECOORDINATOR

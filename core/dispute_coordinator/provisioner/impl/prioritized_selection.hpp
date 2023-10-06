/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PRIORITIZEDSELECTION
#define KAGOME_DISPUTE_PRIORITIZEDSELECTION

#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"
#include "scale/bitvec.hpp"

namespace kagome::runtime {
  class ParachainHost;
}

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::dispute {

  /// The entire state of a dispute.
  struct DisputeState {
    SCALE_TIE(4);

    /// A bitfield indicating all validators for the candidate.
    scale::BitVec validators_for;
    /// A bitfield indicating all validators against the candidate.
    scale::BitVec validators_against;
    /// The block number at which the dispute started on-chain.
    primitives::BlockNumber start;
    /// The block number at which the dispute concluded on-chain.
    std::optional<primitives::BlockNumber> concluded_at;
  };

  /// Contains disputes by partitions. Check the field comments for further
  /// details.
  struct PartitionedDisputes {
    /// Concluded and inactive disputes which are completely unknown for the
    /// Runtime. Hopefully this should never happen.
    /// Will be sent to the Runtime with FIRST priority.
    std::vector<std::tuple<SessionIndex, CandidateHash>>
        inactive_unknown_onchain;
    /// Disputes which are INACTIVE locally but they are unconcluded for the
    /// Runtime. A dispute can have enough local vote to conclude and at the
    /// same time the Runtime knows nothing about them at treats it as
    /// unconcluded. This discrepancy should be treated with high priority.
    /// Will be sent to the Runtime with SECOND priority.
    std::vector<std::tuple<SessionIndex, CandidateHash>>
        inactive_unconcluded_onchain;
    /// Active disputes completely unknown onchain.
    /// Will be sent to the Runtime with THIRD priority.
    std::vector<std::tuple<SessionIndex, CandidateHash>> active_unknown_onchain;
    /// Active disputes unconcluded onchain.
    /// Will be sent to the Runtime with FOURTH priority.
    std::vector<std::tuple<SessionIndex, CandidateHash>>
        active_unconcluded_onchain;
    /// Active disputes concluded onchain. New votes are not that important for
    /// this partition.
    /// Will be sent to the Runtime with FIFTH priority.
    std::vector<std::tuple<SessionIndex, CandidateHash>>
        active_concluded_onchain;
    /// Inactive disputes which has concluded onchain. These are not interesting
    /// and won't be sent to the Runtime.
    /// Will be DROPPED
    std::vector<std::tuple<SessionIndex, CandidateHash>>
        inactive_concluded_onchain;
  };

  /// This module uses different approach for selecting dispute votes. It
  /// queries the Runtime about the votes already known onchain and tries to
  /// select only relevant votes. Refer to the documentation of
  /// `select_disputes` for more details about the actual implementation.
  class PrioritizedSelection final {
   public:
    /// The maximum number of disputes Provisioner will include in the inherent
    /// data.
    /// Serves as a protection not to flood the Runtime with excessive data.
    static constexpr size_t kMaxDisputeVotesForwardedToRuntime = 200'000;

    /// Controls how much dispute votes to be fetched from the
    /// `dispute-coordinator` per iteration in `fn vote_selection`. The purpose
    /// is to fetch the votes in batches until
    /// `kMaxDisputeVotesForwardedToRuntime` is reached. If all votes are
    /// fetched in single call we might fetch votes which we never use. This
    /// will create unnecessary load on `dispute-coordinator`.
    ///
    /// This value should be less than `kMaxDisputeVotesForwardedToRuntime`.
    /// Increase it in case `provisioner` sends too many `QueryCandidateVotes`
    /// messages to `dispute-coordinator`.
    static constexpr size_t kVotesSelectionBatchSize = 1'100;

    PrioritizedSelection(
        clock::SystemClock &clock,
        std::shared_ptr<runtime::ParachainHost> api,
        std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator,
        log::Logger log)
        : clock_(clock),
          api_(std::move(api)),
          dispute_coordinator_(std::move(dispute_coordinator)),
          log_(std::move(log)) {}

    /// Implements the `select_disputes` function which selects dispute votes
    /// which should be sent to the Runtime.
    ///
    /// # How the prioritization works
    ///
    /// Generally speaking disputes can be described as:
    ///   * Active vs Inactive
    ///   * Known vs Unknown onchain
    ///   * Offchain vs Onchain
    ///   * Concluded onchain vs Unconcluded onchain
    ///
    /// Provisioner fetches all disputes from `dispute-coordinator` and
    /// separates them in multiple partitions. Please refer to `struct
    /// PartitionedDisputes` for details about the actual partitions. Each
    /// partition has got a priority implicitly assigned to it and the disputes
    /// are selected based on this priority (e.g. disputes in partition 1, then
    /// if there is space - disputes from partition 2 and so on).
    ///
    /// # Votes selection
    ///
    /// Besides the prioritization described above the votes in each partition
    /// are filtered too. Provisioner fetches all onchain votes and filters them
    /// out from all partitions. As a result the Runtime receives only fresh
    /// votes (votes it didn't know about).
    ///
    /// # How the onchain votes are fetched
    ///
    /// The logic outlined above relies on `RuntimeApiRequest::Disputes` message
    /// from the Runtime. The user check the Runtime version before calling
    /// `select_disputes`. If the function is used with old runtime an error is
    /// logged and the logic will continue with empty onchain votes `HashMap`.
    MultiDisputeStatementSet select_disputes(const primitives::BlockInfo &leaf);

   private:
    /// Selects dispute votes from `PartitionedDisputes` which should be sent to
    /// the runtime. Votes which are already onchain are filtered out. Result
    /// should be sorted by `(SessionIndex, CandidateHash)` which is enforced by
    /// the `BTreeMap`. This is a requirement from the runtime.
    std::map<std::tuple<SessionIndex, CandidateHash>, CandidateVotes>
    vote_selection(PartitionedDisputes partitioned,
                   std::unordered_map<std::tuple<SessionIndex, CandidateHash>,
                                      DisputeState> onchain);

    PartitionedDisputes partition_recent_disputes(
        std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
            recent,
        std::unordered_map<std::tuple<SessionIndex, CandidateHash>,
                           DisputeState> onchain);

    /// Determines if a vote is worth to be kept, based on the onchain disputes
    bool is_vote_worth_to_keep(ValidatorIndex validator_index,
                               DisputeStatement dispute_statement,
                               const DisputeState &onchain_state);

    clock::SystemClock &clock_;
    std::shared_ptr<runtime::ParachainHost> api_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;
    log::Logger log_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PRIORITIZEDSELECTION

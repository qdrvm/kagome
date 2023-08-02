/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_BATCH
#define KAGOME_DISPUTE_BATCH

#include <libp2p/peer/peer_id.hpp>

#include "clock/impl/clock_impl.hpp"
#include "dispute_coordinator/types.hpp"
#include "log/logger.hpp"

namespace kagome::dispute {

  class Batch final {
   public:
    using TimePoint = clock::SteadyClock::TimePoint;

    /// We can have relative large timeouts here, there is no value of hitting a
    /// timeout as we want to get statements through to each node in any case.
    static constexpr auto kDisputeRequestTimeout = std::chrono::seconds(12);

    /// Safe-guard in case votes trickle in real slow.
    ///
    /// If the batch life time exceeded the time the sender is willing to wait
    /// for a confirmation, we would trigger pointless re-sends.
    static constexpr auto kMaxBatchLifetime =
        kDisputeRequestTimeout - std::chrono::seconds(2);

    /// Limit the number of batches that can be alive at any given time.
    ///
    /// Reasoning for this number, see guide.
    static constexpr size_t kMaxBatches = 1000;

    /// Time we allow to pass for new votes to trickle in.
    ///
    /// See `MIN_KEEP_BATCH_ALIVE_VOTES` above.
    /// Should be greater or equal to `RECEIVE_RATE_LIMIT` (there is no point in
    /// checking any faster).
    static constexpr auto kBatchCollectingInterval =
        std::chrono::milliseconds(500);

    /// How many votes must have arrived in the last `BATCH_COLLECTING_INTERVAL`
    ///
    /// in order for a batch to stay alive and not get flushed/imported to the
    /// dispute-coordinator.
    ///
    /// This ensures a timely import of batches.
    static constexpr uint32_t kMinKeepBatchAliveVotes = 10;

    Batch(CandidateHash candidate_hash,
          CandidateReceipt candidate_receipt,
          TimePoint now);

    /// Add votes from a validator into the batch.
    ///
    /// The statements are supposed to be the valid and invalid statements
    /// received in a `DisputeRequest`.
    ///
    /// The given `pending_response` is the corresponding response sender for
    /// responding to `peer`. If at least one of the votes is new as far as this
    /// batch is concerned we record the pending_response, for later use. In
    /// case both votes are known already, we return the response sender as an
    /// `Err` value.
    std::optional<CbOutcome<void>> add_votes(
        Indexed<SignedDisputeStatement> valid_vote,
        Indexed<SignedDisputeStatement> invalid_vote,
        const libp2p::peer::PeerId &peer,
        CbOutcome<void> &&cb);

    /// Check batch for liveness.
    ///
    /// This function is supposed to be called at instants given at construction
    /// and as returned as part of `TickResult`.
    std::optional<PreparedImport> tick(TimePoint now);

    TimePoint next_tick_time() const {
      return next_tick_time_;
    };

    /// Cache of `CandidateHash` (candidate_receipt.hash()).
    CandidateHash candidate_hash;

    /// The actual candidate this batch is concerned with.
    CandidateReceipt candidate_receipt;

   private:
    /// Expiry time for the batch.
    ///
    /// By this time the latest this batch will get flushed.
    const TimePoint best_before_;

    TimePoint next_tick_time_;

    /// All valid votes received in this batch so far.
    ///
    /// We differentiate between valid and invalid votes, so we can detect (and
    /// drop) duplicates, while still allowing validators to equivocate.
    ///
    /// Detecting and rejecting duplicates is crucial in order to effectively
    /// enforce `MIN_KEEP_BATCH_ALIVE_VOTES` per `BATCH_COLLECTING_INTERVAL`. If
    /// we would count duplicates here, the mechanism would be broken.
    std::unordered_map<ValidatorIndex, SignedDisputeStatement> valid_votes_;

    /// All invalid votes received in this batch so far.
    std::unordered_map<ValidatorIndex, SignedDisputeStatement> invalid_votes_;

    /// How many votes have been batched since the last tick/creation.
    size_t votes_batched_since_last_tick_ = 0;

    /// Requesters waiting for a response.
    std::vector<std::tuple<libp2p::peer::PeerId, CbOutcome<void>>> requesters_;

    log::Logger log_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_BATCH

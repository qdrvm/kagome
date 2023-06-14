/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/prioritized_selection.hpp"
#include "utils/tuple_hash.hpp"

namespace kagome::dispute {

  MultiDisputeStatementSet PrioritizedSelection::select_disputes(
      const primitives::BlockInfo &leaf) {
    // SL_TRACE(
    //     log_,
    //     "Selecting disputes for inherent data using prioritized  selection; "
    //     "relay parent {}",
    //     leaf);

    // Fetch the onchain disputes. We'll do a prioritization based on them.
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
        onchain;
    auto onchain_res = get_onchain_disputes(leaf.hash);
    if (onchain_res.has_value()) {
      onchain.swap(onchain_res.value());
    } else {
      // SL_ERROR(log_, "Can't fetch onchain disputes: {}",
      // onchain_res.error());
    }

    /* clang-format off

    // Fetch the onchain disputes. We'll do a prioritization based on them.
    let onchain = match get_onchain_disputes(sender, leaf.hash).await {
      Ok(r) => r,
      Err(GetOnchainDisputesError::NotSupported(runtime_api_err, relay_parent)) => {
        // Runtime version is checked before calling this method, so the error below should never happen!
        SL_ERROR(log_,
          ?runtime_api_err,
          ?relay_parent,
          "because ParachainHost runtime api version is old. Will continue with empty onchain disputes set.",
        );
        HashMap::new()
      },
      Err(GetOnchainDisputesError::Channel) => {
        // This error usually means the node is shutting down. Log just in case.
        SL_DEBUG(log_,
          "Channel error occurred while fetching onchain disputes. Will continue with empty onchain disputes set.",
        );
        HashMap::new()
      },
      Err(GetOnchainDisputesError::Execution(runtime_api_err, parent_hash)) => {
        SL_WARN(log_,
          ?runtime_api_err,
          ?parent_hash,
          "Unexpected execution error occurred while fetching onchain votes. Will continue with empty onchain disputes set.",
        );
        HashMap::new()
      },
    };

    let recent_disputes = request_disputes(sender).await;
    SL_TRACE(log_,
      ?leaf,
      "Got {} recent disputes and {} onchain disputes.",
      recent_disputes.len(),
      onchain.len(),
    );

    // Filter out unconfirmed disputes. However if the dispute is already onchain - don't skip it.
    // In this case we'd better push as much fresh votes as possible to bring it to conclusion faster.
    let recent_disputes = recent_disputes
      .into_iter()
      .filter(|d| d.2.is_confirmed_concluded() || onchain.contains_key(&(d.0, d.1)))
      .collect::<Vec<_>>();

    let partitioned = partition_recent_disputes(recent_disputes, &onchain);
    metrics.on_partition_recent_disputes(&partitioned);

    if partitioned.inactive_unknown_onchain.len() > 0 {
      SL_WARN(log_,
        ?leaf,
        "Got {} inactive unknown onchain disputes. This should not happen!",
        partitioned.inactive_unknown_onchain.len()
      );
    }
    let result = vote_selection(sender, partitioned, &onchain).await;

    make_multi_dispute_statement_set(metrics, result)

    clang-format on */
    return {};
  }

  std::map<std::tuple<SessionIndex, CandidateHash>, CandidateVotes>
  PrioritizedSelection::vote_selection(
      PartitionedDisputes partitioned,
      std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
          onchain) {
    // TODO need to be implemented

    /* clang-format off

    // fetch in batches until there are enough votes
    let mut disputes = partitioned.into_iter().collect::<Vec<_>>();
    let mut total_votes_len = 0;
    let mut result = BTreeMap::new();
    let mut request_votes_counter = 0;
    while !disputes.is_empty() {
      let batch_size = std::cmp::min(VOTES_SELECTION_BATCH_SIZE, disputes.len());
      let batch = Vec::from_iter(disputes.drain(0..batch_size));

      // Filter votes which are already onchain
      request_votes_counter += 1;
      let votes = super::request_votes(sender, batch)
        .await
        .into_iter()
        .map(|(session_index, candidate_hash, mut votes)| {
          let onchain_state =
            if let Some(onchain_state) = onchain.get(&(session_index, candidate_hash)) {
              onchain_state
            } else {
              // onchain knows nothing about this dispute - add all votes
              return (session_index, candidate_hash, votes)
            };

          votes.valid.retain(|validator_idx, (statement_kind, _)| {
            is_vote_worth_to_keep(
              validator_idx,
              DisputeStatement::Valid(*statement_kind),
              &onchain_state,
            )
          });
          votes.invalid.retain(|validator_idx, (statement_kind, _)| {
            is_vote_worth_to_keep(
              validator_idx,
              DisputeStatement::Invalid(*statement_kind),
              &onchain_state,
            )
          });
          (session_index, candidate_hash, votes)
        })
        .collect::<Vec<_>>();

      // Check if votes are within the limit
      for (session_index, candidate_hash, selected_votes) in votes {
        let votes_len = selected_votes.valid.raw().len() + selected_votes.invalid.len();
        if votes_len + total_votes_len > MAX_DISPUTE_VOTES_FORWARDED_TO_RUNTIME {
          // we are done - no more votes can be added. Importantly, we don't add any votes for a dispute here
          // if we can't fit them all. This gives us an important invariant, that backing votes for
          // disputes make it into the provisioned vote set.
          return result
        }
        result.insert((session_index, candidate_hash), selected_votes);
        total_votes_len += votes_len
      }
    }

    SL_TRACE(log_,
      ?request_votes_counter,
      "vote_selection DisputeCoordinatorMessage::QueryCandidateVotes counter",
    );

    result

    clang-format on */
    return {};
  }

  bool PrioritizedSelection::concluded_onchain(DisputeState &onchain_state) {
    // TODO need to be implemented

    /* clang-format off

    // Check if there are enough onchain votes for or against to conclude the dispute
    let supermajority = supermajority_threshold(onchain_state.validators_for.len());

    onchain_state.validators_for.count_ones() >= supermajority ||
      onchain_state.validators_against.count_ones() >= supermajority

    clang-format on */
    return {};
  }

  PartitionedDisputes PrioritizedSelection::partition_recent_disputes(
      std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
          recent,
      std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
          onchain) {
    // TODO need to be implemented

    /* clang-format off

    let mut partitioned = PartitionedDisputes::new();

    // Drop any duplicates
    let unique_recent = recent
      .into_iter()
      .map(|(session_index, candidate_hash, dispute_state)| {
        ((session_index, candidate_hash), dispute_state)
      })
      .collect::<HashMap<_, _>>();

    // Split recent disputes in ACTIVE and INACTIVE
    let time_now = &secs_since_epoch();
    let (active, inactive): (
      Vec<(SessionIndex, CandidateHash, DisputeStatus)>,
      Vec<(SessionIndex, CandidateHash, DisputeStatus)>,
    ) = unique_recent
      .into_iter()
      .map(|((session_index, candidate_hash), dispute_state)| {
        (session_index, candidate_hash, dispute_state)
      })
      .partition(|(_, _, status)| !dispute_is_inactive(status, time_now));

    // Split ACTIVE in three groups...
    for (session_index, candidate_hash, _) in active {
      match onchain.get(&(session_index, candidate_hash)) {
        Some(d) => match concluded_onchain(d) {
          true => partitioned.active_concluded_onchain.push((session_index, candidate_hash)),
          false =>
            partitioned.active_unconcluded_onchain.push((session_index, candidate_hash)),
        },
        None => partitioned.active_unknown_onchain.push((session_index, candidate_hash)),
      };
    }

    // ... and INACTIVE in three more
    for (session_index, candidate_hash, _) in inactive {
      match onchain.get(&(session_index, candidate_hash)) {
        Some(onchain_state) =>
          if concluded_onchain(onchain_state) {
            partitioned.inactive_concluded_onchain.push((session_index, candidate_hash));
          } else {
            partitioned.inactive_unconcluded_onchain.push((session_index, candidate_hash));
          },
        None => partitioned.inactive_unknown_onchain.push((session_index, candidate_hash)),
      }
    }

    partitioned

    clang-format on */
    return {};
  }

  /// Determines if a vote is worth to be kept, based on the onchain disputes
  bool PrioritizedSelection::is_vote_worth_to_keep(
      ValidatorIndex validator_index,
      DisputeStatement dispute_statement,
      DisputeState onchain_state) {
    // TODO need to be implemented

    /* clang-format off

    let (offchain_vote, valid_kind) = match dispute_statement {
      DisputeStatement::Valid(kind) => (true, Some(kind)),
      DisputeStatement::Invalid(_) => (false, None),
    };
    // We want to keep all backing votes. This maximizes the number of backers
    // punished when misbehaving.
    if let Some(kind) = valid_kind {
      match kind {
        ValidDisputeStatementKind::BackingValid(_) |
        ValidDisputeStatementKind::BackingSeconded(_) => return true,
        _ => (),
      }
    }

    let in_validators_for = onchain_state
      .validators_for
      .get(validator_index.0 as usize)
      .as_deref()
      .copied()
      .unwrap_or(false);
    let in_validators_against = onchain_state
      .validators_against
      .get(validator_index.0 as usize)
      .as_deref()
      .copied()
      .unwrap_or(false);

    if in_validators_for && in_validators_against {
      // The validator has double voted and runtime knows about this. Ignore this vote.
      return false
    }

    if offchain_vote && in_validators_against || !offchain_vote && in_validators_for {
      // offchain vote differs from the onchain vote
      // we need this vote to punish the offending validator
      return true
    }

    // The vote is valid. Return true if it is not seen onchain.
    !in_validators_for && !in_validators_against

    clang-format on */
    return {};
  }

  /// Request disputes identified by `CandidateHash` and the `SessionIndex`.
  std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
  PrioritizedSelection::request_disputes() {
    // TODO need to be implemented

    /* clang-format off

    let (tx, rx) = oneshot::channel();
    let msg = DisputeCoordinatorMessage::RecentDisputes(tx);

    // Bounded by block production - `ProvisionerMessage::RequestInherentData`.
    sender.send_unbounded_message(msg);

    let recent_disputes = rx.await.unwrap_or_else(|err| {
      SL_WARN(log_,  err=?err, "Unable to gather recent disputes");
      Vec::new()
    });
    recent_disputes

    clang-format on */
    return {};
  }

  MultiDisputeStatementSet
  PrioritizedSelection::make_multi_dispute_statement_set(
      std::map<std::tuple<SessionIndex, CandidateHash>, CandidateVotes>
          dispute_candidate_votes) {
    // TODO need to be implemented

    /* clang-format off

    // Transform all `CandidateVotes` into `MultiDisputeStatementSet`.
    dispute_candidate_votes
      .into_iter()
      .map(|((session_index, candidate_hash), votes)| {
        let valid_statements = votes
          .valid
          .into_iter()
          .map(|(i, (s, sig))| (DisputeStatement::Valid(s), i, sig));

        let invalid_statements = votes
          .invalid
          .into_iter()
          .map(|(i, (s, sig))| (DisputeStatement::Invalid(s), i, sig));

        metrics.inc_valid_statements_by(valid_statements.len());
        metrics.inc_invalid_statements_by(invalid_statements.len());
        metrics.inc_dispute_statement_sets_by(1);

        DisputeStatementSet {
          candidate_hash,
          session: session_index,
          statements: valid_statements.chain(invalid_statements).collect(),
        }
      })
      .collect()

    clang-format on */
    return {};
  }

  outcome::result<
      std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>>
  PrioritizedSelection::get_onchain_disputes(
      const primitives::BlockHash &relay_parent) {
    // TODO need to be implemented

    /* clang-format off

    SL_TRACE(log_,  ?relay_parent, "Fetching on-chain disputes");
    let (tx, rx) = oneshot::channel();
    sender
      .send_message(RuntimeApiMessage::Request(relay_parent, RuntimeApiRequest::Disputes(tx)))
      .await;

    rx.await
      .map_err(|_| GetOnchainDisputesError::Channel)
      .and_then(|res| {
        res.map_err(|e| match e {
          RuntimeApiError::Execution { .. } =>
            GetOnchainDisputesError::Execution(e, relay_parent),
          RuntimeApiError::NotSupported { .. } =>
            GetOnchainDisputesError::NotSupported(e, relay_parent),
        })
      })
      .map(|v| v.into_iter().map(|e| ((e.0, e.1), e.2)).collect())

    clang-format on */
    std::unordered_map<std::tuple<SessionIndex, CandidateHash>, DisputeState>
        r{};
    return outcome::success(std::move(r));
  }

}  // namespace kagome::dispute

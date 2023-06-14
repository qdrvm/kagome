/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/provisioner/impl/random_selection.hpp"

namespace kagome::dispute {

  std::vector<std::tuple<SessionIndex, CandidateHash>>
  RandomSelection::request_confirmed_disputes(
      RandomSelection::RequestType active_or_recent) {
    // TODO need to be implemented

    /* clang-format off

    let (tx, rx) = oneshot::channel();
    let msg = match active_or_recent {
      RequestType::Recent => DisputeCoordinatorMessage::RecentDisputes(tx),
      RequestType::Active => DisputeCoordinatorMessage::ActiveDisputes(tx),
    };

    sender.send_unbounded_message(msg);
    let disputes = match rx.await {
      Ok(r) => r,
      Err(oneshot::Canceled) => {
        SL_WARN(log_,
          "Channel closed: unable to gather {:?} disputes",
          active_or_recent
        );
        Vec::new()
      },
    };

    disputes
      .into_iter()
      .filter(|d| d.2.is_confirmed_concluded())
      .map(|d| (d.0, d.1))
      .collect()

     clang-format on */
    return {};
  }

  void RandomSelection::extend_by_random_subset_without_repetition(
      std::vector<std::tuple<SessionIndex, CandidateHash>> &acc,
      std::vector<std::tuple<SessionIndex, CandidateHash>> extension,
      size_t n) {
    // TODO need to be implemented

    /* clang-format off

    use rand::Rng;

    let lut = acc.iter().cloned().collect::<HashSet<(SessionIndex, CandidateHash)>>();

    let mut unique_new =
      extension.into_iter().filter(|recent| !lut.contains(recent)).collect::<Vec<_>>();

    // we can simply add all
    if unique_new.len() <= n {
      acc.extend(unique_new)
    } else {
      acc.reserve(n);
      let mut rng = rand::thread_rng();
      for _ in 0..n {
        let idx = rng.gen_range(0..unique_new.len());
        acc.push(unique_new.swap_remove(idx));
      }
    }
    // assure sorting stays candid according to session index
    acc.sort_unstable_by(|a, b| a.0.cmp(&b.0));

    clang-format on */
  }

  MultiDisputeStatementSet RandomSelection::select_disputes() {
    // TODO need to be implemented

    /* clang-format off

    SL_TRACE(log_,  "Selecting disputes for inherent data using random selection");

    // We use `RecentDisputes` instead of `ActiveDisputes` because redundancy is fine.
    // It's heavier than `ActiveDisputes` but ensures that everything from the dispute
    // window gets on-chain, unlike `ActiveDisputes`.
    // In case of an overload condition, we limit ourselves to active disputes, and fill up to the
    // upper bound of disputes to pass to wasm `fn create_inherent_data`.
    // If the active ones are already exceeding the bounds, randomly select a subset.
    let recent = request_confirmed_disputes(sender, RequestType::Recent).await;
    let disputes = if recent.len() > MAX_DISPUTES_FORWARDED_TO_RUNTIME {
      SL_WARN(log_,
        "Recent disputes are excessive ({} > {}), reduce to active ones, and selected",
        recent.len(),
        MAX_DISPUTES_FORWARDED_TO_RUNTIME
      );
      let mut active = request_confirmed_disputes(sender, RequestType::Active).await;
      let n_active = active.len();
      let active = if active.len() > MAX_DISPUTES_FORWARDED_TO_RUNTIME {
        let mut picked = Vec::with_capacity(MAX_DISPUTES_FORWARDED_TO_RUNTIME);
        extend_by_random_subset_without_repetition(
          &mut picked,
          active,
          MAX_DISPUTES_FORWARDED_TO_RUNTIME,
        );
        picked
      } else {
        extend_by_random_subset_without_repetition(
          &mut active,
          recent,
          MAX_DISPUTES_FORWARDED_TO_RUNTIME.saturating_sub(n_active),
        );
        active
      };
      active
    } else {
      recent
    };

    // Load all votes for all disputes from the coordinator.
    let dispute_candidate_votes = super::request_votes(sender, disputes).await;

    // Transform all `CandidateVotes` into `MultiDisputeStatementSet`.
    dispute_candidate_votes
      .into_iter()
      .map(|(session_index, candidate_hash, votes)| {
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

}  // namespace kagome::dispute

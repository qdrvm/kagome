/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"

#include <unordered_set>
#include <vector>

#include "../../../test/testutil/outcome/dummy_error.hpp"
#include "common/visitor.hpp"

namespace kagome::dispute {

  outcome::result<void> DisputeCoordinatorImpl::onImportStatements(
      CandidateHash candidate_hash,
      CandidateReceipt candidate_receipt,
      SessionIndex session,
      std::vector<Vote> statements) {
    handle_statements(candidate_hash, candidate_receipt, session, statements);
  }

  outcome::result<bool> DisputeCoordinatorImpl::handle_statements(
      CandidateHash candidate_hash,
      MaybeCandidateReceipt candidate_receipt,
      SessionIndex session,
      std::vector<Indexed<SignedDisputeStatement>> statements) {
    auto now = clock_->now();

    if (not rolling_session_window_->contains(session)) {
      // It is not valid to participate in an ancient dispute (spam?) or too
      // new.
      return outcome::success(false);
    }

    const auto &candidate_hash_ =
        is_type<CandidateReceipt>(candidate_receipt)
            ? boost::relaxed_get<CandidateReceipt>(candidate_receipt)
                  .commitments_hash
            : boost::relaxed_get<CandidateHash>(candidate_receipt);

    auto session_info_opt = rolling_session_window_->session_info(session));
    if (not session_info_opt.has_value()) {
      SL_DEBUG(
          log_,
          "We are lacking a `SessionInfo` for handling import of statements.");
      return outcome::success(false);
    }
    auto &session_info = session_info_opt.value();

    std::unordered_set<ValidatorIndex> controlled_indices;
    for (ValidatorIndex index = 0; index < session_info.get().validators.size();
         ++index) {
      auto &validator = session_info.get().validators[index];
      if (keystore_->contains(validator)) {
        controlled_indices.emplace(index);
      }
    }

    struct CandidateEnvironment {
      /// The session the candidate appeared in.
      SessionIndex session_index;
      /// Session for above index.
      SessionInfo &session;
      /// Validator indices controlled by this node.
      std::unordered_set<ValidatorIndex> controlled_indices;
    };

    CandidateEnvironment env{.session_index = session,
                             .session = session_info,
                             .controlled_indices = controlled_indices};

    // In case we are not provided with a candidate receipt we operate under the
    // assumption, that a previous vote which included a `CandidateReceipt` was
    // seen. This holds since every block is preceded by the `Backing`-phase.
    //
    // There is one exception: A sufficiently sophisticated attacker could
    // prevent us from seeing the backing votes by withholding arbitrary blocks,
    // and hence we do not have a `CandidateReceipt` available.

    CandidateVoteState old_state;

    auto old_state_opt =
        storage_->load_candidate_votes(session, candidate_hash);
    if (old_state_opt.has_value()) {
      old_state = CandidateVoteState::create(old_state_opt.value(), env, now);
    } else {
      auto provided_opt = if_type<CandidateReceipt>(candidate_receipt);
      if (not provided_opt.has_value()) {
        // LOG "Cannot import votes, without `CandidateReceipt` available!"
        return outcome::success(false);
      }
      old_state = CandidateVoteState {
        .votes =
            CandidateVotes{.candidate_receipt = provided_opt.value().get()},
        .own_vote = OwnVoteState{CannotVote{}}, .dispute_status = std::nullopt
      }
    }

    // LOG "Loaded votes"

    struct ImportResult {
      CandidateVoteState old_state;
      CandidateVoteState new_state;
      size_t imported_invalid_votes;
      size_t imported_valid_votes;
      size_t imported_approval_votes;
      std::vector<ValidatorIndex> new_invalid_voters;
    };

    ImportResult intermediate_result;

    {
      auto votes = std::move(old_state.votes);

      std::vector<ValidatorIndex> new_invalid_voters;
      size_t imported_invalid_votes = 0;
      size_t imported_valid_votes = 0;

      auto expected_candidate_hash = votes.candidate_receipt.commitments_hash;

      for (auto &vote : statements) {
        auto val_index = vote.ix;
        auto &statement = vote.payload;

        if (val_index >= env.session.validators.size()
            or env.session.validators[val_index]
                   != statement.validator_public) {
          // LOG "Validator index doesn't match claimed key"
          continue;
        }

        if (statement.candidate_hash != expected_candidate_hash) {
          // LOG "Vote is for unexpected candidate!"
          continue;
        }

        if (statement.session_index != env.session_index) {
          // LOG "Vote is for unexpected session!"
          continue;
        }

        visit_in_place(
            statement.dispute_statement,
            [&](const ValidDisputeStatement &valid) {
              auto [it, fresh] = votes.valid.emplace(
                  val_index,
                  ValidDisputeStatement{
                      .kind = valid.kind,
                      .index = val_index,
                      .signature = statement.validator_signature});
              if (fresh) {
                ++imported_valid_votes;
                return true;
              }
              auto &existing_kind = it->second.kind;
              return visit_in_place(
                  valid.kind,
                  [&](const Tagged<Empty, struct Explicit> &) {
                    return not is_type<Tagged<Empty, struct Explicit>>(
                        existing_kind);
                  },
                  [&](const Tagged<CandidateHash, struct BackingSeconded> &) {
                    return false;
                  },
                  [&](const Tagged<CandidateHash, struct BackingValid> &) {
                    return false;
                  },
                  [&](const Tagged<Empty, struct ApprovalChecking> &kind) {
                    return not is_type<Tagged<Empty, struct ApprovalChecking>>(
                        existing_kind);
                  });
            },
            [&](const InvalidDisputeStatement &valid) {
              auto [it, fresh] = votes.invalid.emplace(
                  val_index,
                  InvalidDisputeStatement{
                      .kind = valid.kind,
                      .index = val_index,
                      .signature = statement.validator_signature});
              if (fresh) {
                ++imported_invalid_votes;
                new_invalid_voters.push_back(val_index);
                return true;
              }
            });

        CandidateVoteState new_state =
            CandidateVoteState::create(votes, env, now);

        intermediate_result = ImportResult{old_state,
                                           new_state,
                                           imported_invalid_votes,
                                           imported_valid_votes,
                                           0,
                                           new_invalid_voters};
      }

      // Handle approval vote import:
      //
      // See guide: We import on fresh disputes to maximize likelihood of
      // fetching votes for dead forks and once concluded to maximize time for
      // approval votes to trickle in.

      ImportResult import_result;

      auto is_freshly_disputed =
          not intermediate_result.old_state.dispute_status.has_value()
          and intermediate_result.new_state.dispute_status.has_value();
      auto is_freshly_concluded_for =
          intermediate_result.old_state.dispute_status.has_value()
              ? is_type<Tagged<Timestamp, struct ConcludedFor>>(
                  intermediate_result.old_state.dispute_status.value())
              : false;
      auto is_freshly_concluded_against =
          intermediate_result.old_state.dispute_status.has_value()
              ? is_type<Tagged<Timestamp, struct ConcludedAgainst>>(
                  intermediate_result.old_state.dispute_status.value())
              : false;
      auto is_freshly_concluded =
          is_freshly_concluded_for or is_freshly_concluded_against;

      if (is_freshly_disputed or is_freshly_concluded) {
        // LOG "Requesting approval signatures"

        // Use of unbounded channels justified because:
        // 1. Only triggered twice per dispute.
        // 2. Raising a dispute is costly (requires validation + recovery) by
        // honest nodes, dishonest nodes are limited by spam slots.
        // 3. Concluding a dispute is even more costly.
        // Therefore it is reasonable to expect a simple vote request to succeed
        // way faster than disputes are raised.
        // 4. We are waiting (and blocking the whole subsystem) on a response
        // right after - therefore even with all else failing we will never have
        // more than one message in flight at any given time.

        // see: {polkadot}/node/core/dispute-coordinator/src/initialized.rs:809
        auto getApprovalSignaturesForCandidate =  // TODO IT IS STAB!!
            [](const CandidateHash &) -> outcome::result<CandidateVotes> {
          return testutil::DummyError::ERROR;
        };

        auto res = getApprovalSignaturesForCandidate(candidate_hash);
        if (res.has_error()) {
          // LOG "Fetch for approval votes got cancelled, only expected during
          // shutdown!"
          import_result = intermediate_result;
        } else {
          // LOG "Successfully received approval votes."
          auto &votes = res.value();

          intermediate_result.import_approval_votes(  // TODO unstub
              &env,
              votes,
              now);

          import_result = intermediate_result;
        }

      } else {
        // LOG "Not requested approval signatures"
        import_result = intermediate_result;
      }

      // LOG "Import result ready"

      auto &new_state = import_result.new_state;

      auto is_included = scraper_->is_candidate_included(candidate_hash);
      auto is_backed = scraper_->is_candidate_backed(candidate_hash);
      auto own_vote_missing =
          is_type<CannotVote>(new_state.own_vote)
          or boost::relaxed_get<Voted>(new_state.own_vote).empty();
      auto is_disputed = new_state.dispute_status.has_value();
      auto is_confirmed = is_disputed
                            ? is_type<Tagged<Empty, struct Confirmed>>(
                                new_state.dispute_status.value())
                            : false;
      auto potential_spam = is_potential_spam(new_state, candidate_hash);

      // We participate only in disputes which are not potential spam.
      auto allow_participation = not potential_spam;

      // This check is responsible for all clearing of spam slots. It runs
      // whenever a vote is imported from on or off chain, and decrements
      // slots whenever a candidate is newly backed, confirmed, or has our
      // own vote.
      if (not potential_spam) {
        spam_slots_->clear(session, candidate_hash);

        // Potential spam:
      } else if (not import_result.new_invalid_voters.empty()) {
        auto free_spam_slots_available = false;

        // Only allow import if at least one validator voting invalid, has not
        // exceeded its spam slots:
        for (auto validator : import_result.new_invalid_voters) {
          // Disputes can only be triggered via an invalidity stating vote, thus
          // we only need to increase spam slots on invalid votes. (If we did
          // not, we would also increase spam slots for backing validators for
          // example - as validators have to provide some opposing vote for
          // dispute-distribution).
          free_spam_slots_available |=
              spam_slots_->add_unconfirmed(session, candidate_hash, validator);
        }
        if (not free_spam_slots_available) {
          // LOG "Rejecting import because of full spam slots."
          return outcome::success(false);
        }
      }

      // clang-format off


		// Participate in dispute if we did not cast a vote before and actually have keys to cast a
		// local vote. Disputes should fall in one of the categories below, otherwise we will refrain
		// from participation:
		// - `is_included` lands in prioritised queue
		// - `is_confirmed` | `is_backed` lands in best effort queue
		// We don't participate in disputes on finalized candidates.
		if own_vote_missing && is_disputed && allow_participation {
			let priority = ParticipationPriority::with_priority_if(is_included);
			gum::trace!(
				target: LOG_TARGET,
				?candidate_hash,
				?priority,
				"Queuing participation for candidate"
			);
			if priority.is_priority() {
				self.metrics.on_queued_priority_participation();
			} else {
				self.metrics.on_queued_best_effort_participation();
			}
			let r = self
				.participation
				.queue_participation(
					ctx,
					priority,
					ParticipationRequest::new(new_state.candidate_receipt().clone(), session),
				)
				.await;
			log_error(r)?;
		} else {
			gum::trace!(
				target: LOG_TARGET,
				?candidate_hash,
				?is_confirmed,
				?own_vote_missing,
				?is_disputed,
				?allow_participation,
				?is_included,
				?is_backed,
				"Will not queue participation for candidate"
			);

			if !allow_participation {
				self.metrics.on_refrained_participation();
			}
		}

		// Also send any already existing approval vote on new disputes:
		if import_result.is_freshly_disputed() {
			let our_approval_votes = new_state.own_approval_votes().into_iter().flatten();
			for (validator_index, sig) in our_approval_votes {
				let pub_key = match env.validators().get(validator_index) {
					None => {
						gum::error!(
							target: LOG_TARGET,
							?validator_index,
							?session,
							"Could not find pub key in `SessionInfo` for our own approval vote!"
						);
						continue
					},
					Some(k) => k,
				};
				let statement = SignedDisputeStatement::new_unchecked_from_trusted_source(
					DisputeStatement::Valid(ValidDisputeStatementKind::ApprovalChecking),
					candidate_hash,
					session,
					pub_key.clone(),
					sig.clone(),
				);
				gum::trace!(
					target: LOG_TARGET,
					?candidate_hash,
					?session,
					?validator_index,
					"Sending out own approval vote"
				);
				match make_dispute_message(
					env.session_info(),
					&new_state.votes(),
					statement,
					validator_index,
				) {
					Err(err) => {
						gum::error!(
							target: LOG_TARGET,
							?err,
							"No ongoing dispute, but we checked there is one!"
						);
					},
					Ok(dispute_message) => {
						ctx.send_message(DisputeDistributionMessage::SendDispute(dispute_message))
							.await;
					},
				};
			}
		}

		// All good, update recent disputes if state has changed:
		if let Some(new_status) = new_state.dispute_status() {
			// Only bother with db access, if there was an actual change.
			if import_result.dispute_state_changed() {
				let mut recent_disputes = overlay_db.load_recent_disputes()?.unwrap_or_default();

				let status =
					recent_disputes.entry((session, candidate_hash)).or_insert_with(|| {
						gum::info!(
							target: LOG_TARGET,
							?candidate_hash,
							session,
							"New dispute initiated for candidate.",
						);
						DisputeStatus::active()
					});

				*status = *new_status;

				gum::trace!(
					target: LOG_TARGET,
					?candidate_hash,
					?status,
					has_concluded_for = ?new_state.has_concluded_for(),
					has_concluded_against = ?new_state.has_concluded_against(),
					"Writing recent disputes with updates for candidate"
				);
				overlay_db.write_recent_disputes(recent_disputes);
			}
		}

		// Notify ChainSelection if a dispute has concluded against a candidate. ChainSelection
		// will need to mark the candidate's relay parent as reverted.
		if import_result.is_freshly_concluded_against() {
			let blocks_including = self.scraper.get_blocks_including_candidate(&candidate_hash);
			if blocks_including.len() > 0 {
				ctx.send_message(ChainSelectionMessage::RevertBlocks(blocks_including)).await;
			} else {
				gum::debug!(
					target: LOG_TARGET,
					?candidate_hash,
					?session,
					"Could not find an including block for candidate against which a dispute has concluded."
				);
			}
		}

		// Update metrics:
		if import_result.is_freshly_disputed() {
			self.metrics.on_open();
		}
		self.metrics.on_valid_votes(import_result.imported_valid_votes());
		self.metrics.on_invalid_votes(import_result.imported_invalid_votes());
		gum::trace!(
			target: LOG_TARGET,
			?candidate_hash,
			?session,
			imported_approval_votes = ?import_result.imported_approval_votes(),
			imported_valid_votes = ?import_result.imported_valid_votes(),
			imported_invalid_votes = ?import_result.imported_invalid_votes(),
			total_valid_votes = ?import_result.new_state().votes().valid.raw().len(),
			total_invalid_votes = ?import_result.new_state().votes().invalid.len(),
			confirmed = ?import_result.new_state().is_confirmed(),
			"Import summary"
		);

		self.metrics.on_approval_votes(import_result.imported_approval_votes());
		if import_result.is_freshly_concluded_for() {
			gum::info!(
				target: LOG_TARGET,
				?candidate_hash,
				session,
				"Dispute on candidate concluded with 'valid' result",
			);
			self.metrics.on_concluded_valid();
		}
		if import_result.is_freshly_concluded_against() {
			gum::info!(
				target: LOG_TARGET,
				?candidate_hash,
				session,
				"Dispute on candidate concluded with 'invalid' result",
			);
			self.metrics.on_concluded_invalid();
		}

		// Only write when votes have changed.
		if let Some(votes) = import_result.into_updated_votes() {
			overlay_db.write_candidate_votes(session, candidate_hash, votes.into());
		}

		Ok(ImportStatementsResult::ValidImport)










				let report = move || match pending_confirmation {
					Some(pending_confirmation) => pending_confirmation
						.send(outcome)
						.map_err(|_| JfyiError::DisputeImportOneshotSend),
					None => Ok(()),
				};


      return outcome::success(true);
    }
  }
}

}
}

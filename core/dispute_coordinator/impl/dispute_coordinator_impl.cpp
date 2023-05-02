/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"

#include <unordered_set>
#include <vector>

#include "../../../test/testutil/outcome/dummy_error.hpp"
#include "application/app_state_manager.hpp"
#include "common/visitor.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/participation/participation.hpp"

namespace kagome::dispute {

  DisputeCoordinatorImpl::DisputeCoordinatorImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<LocalKeystore> keystore,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<Storage> storage,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider)
      : app_state_manager_(std::move(app_state_manager)),
        scheduler_(std::move(scheduler)),
        clock_(std::move(clock)),
        keystore_(std::move(keystore)),
        session_keys_(std::move(session_keys)),
        storage_(std::move(storage)),
        sr25519_crypto_provider_(std::move(sr25519_crypto_provider)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(clock_ != nullptr);
    BOOST_ASSERT(keystore_ != nullptr);
    BOOST_ASSERT(session_keys_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(sr25519_crypto_provider_ != nullptr);
  }

  bool DisputeCoordinatorImpl::start() {
    processing_loop_handle_ = scheduler_->scheduleWithHandle([&] { run(); });
    return true;
  }

  void DisputeCoordinatorImpl::stop() {
    processing_loop_handle_.cancel();
  }

  void DisputeCoordinatorImpl::run() {
    auto res = run_until_error();
    if (not res.has_error()) {
      // LOG-INFO "received `Conclude` signal, exiting"
      return;
    }

    // LOG-ERROR "Error happened: " res.error

    if (app_state_manager_->state()
        < application::AppStateManager::State::ShuttingDown) {
      std::ignore =
          processing_loop_handle_.reschedule(std::chrono::milliseconds(0));
    }
  }

  outcome::result<void> DisputeCoordinatorImpl::run_until_error() {
    std::vector<std::pair<ParticipationPriority, ParticipationRequest>>
        participation;
    std::vector<ScrapedOnChainVotes> on_chain_votes;
    std::optional<ActivatedLeaf> first_leaf;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L166
    for (const auto &[priority, request] : std::move(participation)) {
      OUTCOME_TRY(participation_->queue_participation(priority, request));
    }

    {
      // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L160
      // auto  overlay_db = OverlayedBackend::new (backend);
      for (auto votes : on_chain_votes) {
        auto res = process_on_chain_votes(votes);
        if (res.has_error()) {
          // LOG-WARN: "Skipping scraping block due to error"
        }
      }
      // if !overlay_db {
      //   .is_empty() {
      //     let ops = overlay_db.into_write_ops();
      //     backend.write(ops) ? ;
      //   }
      // }
    }

    if (first_leaf.has_value()) {
      // Also provide first leaf to participation for good measure.
      participation_
          ->process_active_leaves_update(
              ctx, &ActiveLeavesUpdate::start_work(first_leaf))
          .await
          ? ;
    }

    /*



        loop {
          gum::trace!(target: LOG_TARGET, "Waiting for message");
          let mut overlay_db = OverlayedBackend::new(backend);
          let default_confirm = Box::new(|| Ok(()));
          let confirm_write = match MuxedMessage::receive(ctx, &mut
       self.participation_receiver) .await?
          {
            MuxedMessage::Participation(msg) => {
              gum::trace!(target: LOG_TARGET, "MuxedMessage::Participation");
              let ParticipationStatement {
                session,
                candidate_hash,
                candidate_receipt,
                outcome,
              } = self.participation.get_participation_result(ctx, msg).await?;
              if let Some(valid) = outcome.validity() {
                gum::trace!(
                  target: LOG_TARGET,
                  ?session,
                  ?candidate_hash,
                  ?valid,
                  "Issuing local statement based on participation outcome."
                );
                self.issue_local_statement(
                  ctx,
                  &mut overlay_db,
                  candidate_hash,
                  candidate_receipt,
                  session,
                  valid,
                  clock.now(),
                )
                .await?;
              } else {
                gum::warn!(target: LOG_TARGET, ?outcome, "Dispute participation
       failed");
              }
              default_confirm
            },
            MuxedMessage::Subsystem(msg) => match msg {
              FromOrchestra::Signal(OverseerSignal::Conclude) => return Ok(()),
              FromOrchestra::Signal(OverseerSignal::ActiveLeaves(update)) => {
                gum::trace!(target: LOG_TARGET, "OverseerSignal::ActiveLeaves");
                self.process_active_leaves_update(
                  ctx,
                  &mut overlay_db,
                  update,
                  clock.now(),
                )
                .await?;
                default_confirm
              },
              FromOrchestra::Signal(OverseerSignal::BlockFinalized(_, n)) => {
                gum::trace!(target: LOG_TARGET,
       "OverseerSignal::BlockFinalized");
                self.scraper.process_finalized_block(&n);
                default_confirm
              },
              FromOrchestra::Communication { msg } =>
                self.handle_incoming(ctx, &mut overlay_db, msg,
       clock.now()).await?,
            },
          };

          if !overlay_db.is_empty() {
            let ops = overlay_db.into_write_ops();
            backend.write(ops)?;
          }
          // even if the changeset was empty,
          // otherwise the caller will error.
          confirm_write()?;
        }
     */

    return outcome::success();
  }

  outcome::result<void> DisputeCoordinatorImpl::process_on_chain_votes(
      ScrapedOnChainVotes votes) {
    const auto &session = votes.session;
    const auto &backing_validators_per_candidate =
        votes.backing_validators_per_candidate;
    const auto &disputes = votes.disputes;

    if (backing_validators_per_candidate.empty() and disputes.empty()) {
      return outcome::success();
    }

    // Obtain the session info, for sake of `ValidatorId`s either from the
    // rolling session window. Must be called _after_ `fn
    // cache_session_info_for_head` which guarantees that the session info is
    // available for the current session.
    auto session_info_opt = rolling_session_window_->session_info(session);
    if (not session_info_opt.has_value()) {
      // LOG-WARN: "Could not retrieve session info from rolling session window"
      return outcome::success();
    }
    auto &session_info = session_info_opt.value().get();

    // Scraped on-chain backing votes for the candidates with the new active
    // leaf as if we received them via gossip.
    for (auto &[candidate_receipt, backers] :
         backing_validators_per_candidate) {
      auto &relay_parent = candidate_receipt.descriptor.relay_parent;
      auto &candidate_hash = candidate_receipt.commitments_hash;

      // LOG-TRACE: "Importing backing votes from chain for candidate"

      std::vector<Indexed<SignedDisputeStatement>> statements;
      for (auto &[validator_index, attestation] : backers) {
        if (validator_index >= session_info.validators.size()) {
          // LOG-ERR: "Missing public key for validator"
          return SignatureValidationError::MissingPublicKey;
        }

        ValidatorId validator_public = session_info.validators[validator_index];

        ValidatorSignature validator_signature = visit_in_place(
            attestation,
            [](const Unused<0> &) { return ValidatorSignature{}; },
            [](const auto &sig) { return (ValidatorSignature)sig; });

        auto valid_statement_kind = visit_in_place(
            attestation,
            [](const Unused<0> &) { return ValidDisputeStatementKind{}; },
            [&](const ImplicitValidityAttestation &) {
              return ValidDisputeStatementKind(BackingSeconded(relay_parent));
            },
            [&](const ExplicitValidityAttestation &) {
              return ValidDisputeStatementKind(BackingValid(relay_parent));
            });

        auto check_sig = [&, validator_index = validator_index]() -> bool {
          ValidDisputeStatement statement{
              valid_statement_kind, validator_index, validator_signature};

          auto payload = getPayload(statement, candidate_hash, session);

          auto validation_res = sr25519_crypto_provider_->verify(
              validator_signature, payload, validator_public);

          if (validation_res.has_error()) {
            return false;
          }
          if (not validation_res.value()) {
            return false;
          }
          return true;
        };

        BOOST_ASSERT_MSG(check_sig(),
                         "Scraped backing votes had invalid signature!");

        SignedDisputeStatement signed_dispute_statement{
            ValidDisputeStatement{
                valid_statement_kind, session, validator_signature},
            candidate_hash,
            validator_public,
            validator_signature,
            session,
        };

        statements.emplace_back(signed_dispute_statement, validator_index);
      }

      // Importantly, handling import statements for backing votes also
      // clears spam slots for any newly backed candidates
      OUTCOME_TRY(
          import_result,
          handle_import_statements(candidate_receipt, session, statements));

      if (import_result) {
        // LOG-TRACE: "Imported backing votes from chain"
      } else {
        // LOG-WARN: "Attempted import of on-chain backing votes failed"
      }
    }

    // Import disputes from on-chain, this already went through a vote so it's
    // assumed as verified. This will only be stored, gossiping it is not
    // necessary.

    // First try to obtain all the backings which ultimately contain the
    // candidate receipt which we need.

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L461
    for (auto &dispute_statement_set : disputes) {
      auto &candidate_hash = dispute_statement_set.candidate_hash;
      auto &session_ = dispute_statement_set.session;
      auto &dispute_statements = dispute_statement_set.statements;
      // LOG-TRACE: "Importing dispute votes from chain for candidate"

      std::vector<Indexed<SignedDisputeStatement>> statements;
      for (const auto &statement : dispute_statements) {
        OUTCOME_TRY(visit_in_place(
            statement, [&](const auto &statement) -> outcome::result<void> {
              auto &valid_statement_kind = statement.kind;
              auto &validator_index = statement.index;
              auto &validator_signature = statement.signature;

              auto session_info_opt =
                  rolling_session_window_->session_info(session_);
              if (not session_info_opt.has_value()) {
                // LOG-WARN: "Could not retrieve session info from rolling
                // session window for recently concluded dispute"
                return outcome::success();
              }
              auto &session_info = session_info_opt.value().get();

              if (validator_index >= session_info.validators.size()) {
                // LOG-ERR: "Missing public key for validator {:?} that
                // participated in concluded dispute"
                return SignatureValidationError::MissingPublicKey;
              }

              ValidatorId validator_public =
                  session_info.validators[validator_index];

              auto check_sig = [&]() -> bool {
                ValidDisputeStatement statement{
                    valid_statement_kind, validator_index, validator_signature};

                auto payload = getPayload(statement, candidate_hash, session);

                auto validation_res = sr25519_crypto_provider_->verify(
                    validator_signature, payload, validator_public);

                if (validation_res.has_error()) {
                  return false;
                }
                if (not validation_res.value()) {
                  return false;
                }
                return true;
              };

              BOOST_ASSERT_MSG(check_sig(),
                               "Scraped dispute votes had invalid signature!");

              SignedDisputeStatement signed_dispute_statement{
                  ValidDisputeStatement{
                      valid_statement_kind, session, validator_signature},
                  candidate_hash,
                  validator_public,
                  validator_signature,
                  session,
              };

              statements.emplace_back(signed_dispute_statement,
                                      validator_index);
            }));
      }

      OUTCOME_TRY(
          import_result,
          handle_import_statements(candidate_hash, session, statements));

      if (import_result) {
        // LOG-TRACE: "Imported statement of dispute from on-chain"
      } else {
        // LOG-WARN: "Attempted import of on-chain statement of dispute failed"
      }
    }

    return outcome::success();
  }

  outcome::result<void> DisputeCoordinatorImpl::process_active_leaves_update(
      ActiveLeavesUpdate update) {
    auto now = clock_->nowUint64();

    /*
    let scraped_updates =
      self.scraper.process_active_leaves_update(ctx.sender(), &update).await?;
    log_error(
      self.participation
        .bump_to_priority_for_candidates(ctx,
    &scraped_updates.included_receipts) .await,
    )?;
    self.participation.process_active_leaves_update(ctx, &update).await?;

    if let Some(new_leaf) = update.activated {
      match self
        .rolling_session_window
        .cache_session_info_for_head(ctx.sender(), new_leaf.hash)
        .await
      {
        Err(e) => {
          gum::warn!(
            target: LOG_TARGET,
            err = ?e,
            "Failed to update session cache for disputes",
          );
          self.error = Some(e);
        },
        Ok(SessionWindowUpdate::Advanced {
          new_window_end: window_end,
          new_window_start,
          ..
        }) => {
          self.error = None;
          let session = window_end;
          if self.highest_session < session {
     gum::trace!(target: LOG_TARGET, session, "Observed new session.  Pruning");

            self.highest_session = session;

            db::v1::note_earliest_session(overlay_db, new_window_start)?;
            self.spam_slots.prune_old(new_window_start);
          }
        },
        Ok(SessionWindowUpdate::Unchanged) => {},
      };

      // The `runtime-api` subsystem has an internal queue which serializes
    the execution,
      // so there is no point in running these in parallel.
      for votes in scraped_updates.on_chain_votes {
        let _ = self.process_on_chain_votes(ctx, overlay_db, votes,
    now).await.map_err( |error| { gum::warn!( target: LOG_TARGET, ?error,
              "Skipping scraping block due to error",
            );
          },
        );
      }
    }

     */

    return outcome::success();
  }

  outcome::result<void> DisputeCoordinatorImpl::onImportStatements(
      CandidateReceipt candidate_receipt,
      SessionIndex session,
      std::vector<Indexed<SignedDisputeStatement>> statements,
      std::optional<std::function<void(outcome::result<void>)>>
          pending_confirmation) {
    // see:https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L563
    OUTCOME_TRY(
        handle_import_statements(candidate_receipt, session, statements));

    return outcome::success();
  }

  outcome::result<bool> DisputeCoordinatorImpl::handle_import_statements(
      MaybeCandidateReceipt candidate_receipt,
      const SessionIndex session,
      std::vector<Indexed<SignedDisputeStatement>> statements) {
    auto now = clock_->nowUint64();

    if (not rolling_session_window_->contains(session)) {
      // It is not valid to participate in an ancient dispute (spam?) or too
      // new.
      return outcome::success(false);
    }

    const auto &candidate_hash =
        is_type<CandidateReceipt>(candidate_receipt)
            ? boost::relaxed_get<CandidateReceipt>(candidate_receipt)
                  .commitments_hash
            : boost::relaxed_get<CandidateHash>(candidate_receipt);

    auto session_info_opt = rolling_session_window_->session_info(session);
    if (not session_info_opt.has_value()) {
      SL_DEBUG(log_,
               "We are lacking a `SessionInfo` for handling import of "
               "statements.");
      return outcome::success(false);
    }
    auto &session_info = session_info_opt.value().get();

    std::unordered_set<ValidatorIndex> controlled_indices;
    auto &keypair = session_keys_->getParaKeyPair();
    if (keypair != nullptr) {
      for (ValidatorIndex index = 0; index < session_info.validators.size();
           ++index) {
        auto &validator = session_info.validators[index];

        if (keypair->public_key == validator) {
          controlled_indices.emplace(index);
        }
      }
    }

    CandidateEnvironment env{.session_index = session,
                             .session = session_info,
                             .controlled_indices = controlled_indices};

    // In case we are not provided with a candidate receipt we operate under
    // the assumption, that a previous vote which included a
    // `CandidateReceipt` was seen. This holds since every block is preceded
    // by the `Backing`-phase.
    //
    // There is one exception: A sufficiently sophisticated attacker could
    // prevent us from seeing the backing votes by withholding arbitrary
    // blocks, and hence we do not have a `CandidateReceipt` available.

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
      old_state = {
          .votes = {.candidate_receipt = provided_opt.value().get()},
          .own_vote = {CannotVote{}},
          .dispute_status = std::nullopt,
      };
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
                  [&](const Explicit &) {
                    return not is_type<Explicit>(existing_kind);
                  },
                  [&](const BackingSeconded &) { return false; },
                  [&](const BackingValid &) { return false; },
                  [&](const ApprovalChecking &kind) {
                    return not is_type<ApprovalChecking>(existing_kind);
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
              return false;
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

    auto is_old_concluded_for =
        intermediate_result.old_state.dispute_status.has_value()
            ? is_type<ConcludedFor>(
                intermediate_result.old_state.dispute_status.value())
            : false;
    auto is_new_concluded_for =
        intermediate_result.new_state.dispute_status.has_value()
            ? is_type<ConcludedFor>(
                intermediate_result.new_state.dispute_status.value())
            : false;
    auto is_freshly_concluded_for =
        not is_old_concluded_for and is_new_concluded_for;

    auto is_old_concluded_against =
        intermediate_result.old_state.dispute_status.has_value()
            ? is_type<ConcludedAgainst>(
                intermediate_result.old_state.dispute_status.value())
            : false;
    auto is_new_concluded_against =
        intermediate_result.new_state.dispute_status.has_value()
            ? is_type<ConcludedAgainst>(
                intermediate_result.new_state.dispute_status.value())
            : false;
    auto is_freshly_concluded_against =
        not is_old_concluded_against and is_new_concluded_against;

    auto is_freshly_concluded =
        is_freshly_concluded_for or is_freshly_concluded_against;

    auto is_old_confirmed_concluded =
        intermediate_result.old_state.dispute_status.has_value()
            ? not is_type<Active>(
                intermediate_result.old_state.dispute_status.value())
            : false;
    auto is_new_confirmed_concluded =
        intermediate_result.new_state.dispute_status.has_value()
            ? not is_type<Active>(
                intermediate_result.new_state.dispute_status.value())
            : false;
    auto is_freshly_confirmed =
        not is_old_confirmed_concluded and is_new_confirmed_concluded;

    if (is_freshly_disputed or is_freshly_concluded) {
      // LOG "Requesting approval signatures"

      // Use of unbounded channels justified because:
      // 1. Only triggered twice per dispute.
      // 2. Raising a dispute is costly (requires validation + recovery) by
      // honest nodes, dishonest nodes are limited by spam slots.
      // 3. Concluding a dispute is even more costly.
      // Therefore it is reasonable to expect a simple vote request to
      // succeed way faster than disputes are raised.
      // 4. We are waiting (and blocking the whole subsystem) on a response
      // right after - therefore even with all else failing we will never
      // have more than one message in flight at any given time.

      // see:
      // {polkadot}/node/core/dispute-coordinator/src/initialized.rs:809
      auto getApprovalSignaturesForCandidate =  // TODO IT IS STAB!!
          [](const CandidateHash &)
          -> outcome::result<
              std::unordered_map<ValidatorIndex, ValidatorSignature>> {
        return testutil::DummyError::ERROR;
      };

      auto res = getApprovalSignaturesForCandidate(candidate_hash);
      if (res.has_error()) {
        // LOG "Fetch for approval votes got cancelled, only expected during
        // shutdown!"
        import_result = std::move(intermediate_result);
      } else {
        // LOG "Successfully received approval votes."
        auto &approval_votes = res.value();

        // import approval votes

        import_result = std::move(intermediate_result);

        auto _votes = std::move(intermediate_result.new_state.votes);

        for (auto &[index, signature] : approval_votes) {
          // clang-format off
            BOOST_ASSERT_MSG(
                [&] {
                // see: {polkadot}node/core/dispute-coordinator/src/import.rs:490
                // let pub_key = &env.session_info().validators.get(index).expect("indices are validated by approval-voting subsystem; qed");
                // let candidate_hash = votes.candidate_receipt.hash();
                // let session_index = env.session_index();
                // DisputeStatement::Valid(ValidDisputeStatementKind::ApprovalChecking)
                //   .check_signature(pub_key, candidate_hash, session_index, &sig)
                //   .is_ok();
                  return true;  // FIXME
                }(),
                "Signature check for imported approval votes failed! "
                "This is a serious bug.");
          // clang-format on

          /// Insert a vote, replacing any already existing vote.
          ///
          /// Except, for backing votes: Backing votes are always kept, and
          /// will never get overridden. Import of other king of `valid`
          /// votes, will be ignored if a backing vote is already present.
          /// Any already existing `valid` vote, will be overridden by any
          /// given backing vote.

          bool affected = false;
          if (auto it = _votes.valid.find(index); it == _votes.valid.end()) {
            _votes.valid.emplace(
                index,
                ValidDisputeStatement{.kind = ApprovalChecking{},
                                      .index = index,
                                      .signature = signature});
            affected = true;
          } else {
            if (is_type<BackingValid>(it->second.kind)
                or is_type<BackingSeconded>(it->second.kind)) {
              affected = false;
            } else if (is_type<Explicit>(it->second.kind)
                       or is_type<ApprovalChecking>(it->second.kind)) {
              affected = not is_type<ApprovalChecking>(it->second.kind);
              it->second = ValidDisputeStatement{.kind = ApprovalChecking{},
                                                 .index = index,
                                                 .signature = signature};
            }
          }

          if (affected) {
            import_result.imported_valid_votes += 1;
            import_result.imported_approval_votes += 1;
          }
        }

        import_result.new_state =
            CandidateVoteState::create(std::move(_votes), env, now);
      }
    } else {
      // LOG "Not requested approval signatures"
      import_result = intermediate_result;
    }

    // LOG "Import result ready"

    auto &new_state = import_result.new_state;

    auto is_included = scraper_->is_candidate_included(candidate_hash);
    // auto is_backed = scraper_->is_candidate_backed(candidate_hash);
    auto own_vote_missing =
        is_type<CannotVote>(new_state.own_vote)
        or boost::relaxed_get<Voted>(new_state.own_vote).empty();
    auto is_disputed = new_state.dispute_status.has_value();
    // auto is_confirmed = is_disputed
    //                    ?
    //                    is_type<Confirmed>(new_state.dispute_status.value())
    //                    : false;
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
        // Disputes can only be triggered via an invalidity stating vote,
        // thus we only need to increase spam slots on invalid votes. (If we
        // did not, we would also increase spam slots for backing validators
        // for example - as validators have to provide some opposing vote
        // for dispute-distribution).
        free_spam_slots_available |=
            spam_slots_->add_unconfirmed(session, candidate_hash, validator);
      }
      if (not free_spam_slots_available) {
        // LOG "Rejecting import because of full spam slots."
        return outcome::success(false);
      }
    }

    // Participate in dispute if we did not cast a vote before and actually
    // have keys to cast a local vote. Disputes should fall in one of the
    // categories below, otherwise we will refrain from participation:
    // - `is_included` lands in prioritised queue
    // - `is_confirmed` | `is_backed` lands in best effort queue
    // We don't participate in disputes on finalized candidates.
    // see: {polkadot}/node/core/dispute-coordinator/src/initialized.rs:907

    if (own_vote_missing && is_disputed && allow_participation) {
      auto priority = static_cast<ParticipationPriority>(is_included);

      auto &receipt = new_state.votes.candidate_receipt;

      ParticipationRequest request{.candidate_hash = receipt.commitments_hash,
                                   .candidate_receipt = receipt,
                                   .session = session};

      auto res = participation_->queue_participation(priority, request);
      if (res.has_error()) {
        // LOG: participation error
      }

    } else {
      // LOG: "Will not queue participation for candidate"
    }

    // Also send any already existing approval vote on new disputes:
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L947
    if (not import_result.old_state.dispute_status.has_value()
        and import_result.new_state.dispute_status.has_value()) {
      if (is_type<Voted>(new_state.own_vote)) {
        auto &own_votes = boost::relaxed_get<Voted>(new_state.own_vote);

        for (auto &[validator_index, dispute_statement, sig] : own_votes) {
          auto valid_dispute_statement_opt =
              if_type<ValidDisputeStatement>(dispute_statement);
          if (valid_dispute_statement_opt.has_value()) {
            continue;
          }
          auto &valid_dispute_statement =
              valid_dispute_statement_opt.value().get();
          if (is_type<ApprovalChecking>(valid_dispute_statement.kind)) {
            if (validator_index >= session_info.validators.size()) {
              SL_DEBUG(log_,
                       "Could not find pub key in `SessionInfo` for our own "
                       "approval vote!");
              continue;
            }
            auto &pub_key = session_info.validators[validator_index];

            SignedDisputeStatement statement{
                ValidDisputeStatement{ApprovalChecking{}, {}, {}},  // FIXME
                candidate_hash,
                pub_key,
                sig,
                session};

            // LOG-TRACE: "Sending out own approval vote"

            auto dispute_message_res = make_dispute_message(
                session_info, new_state.votes, statement, validator_index);

            if (dispute_message_res.has_value()) {
              auto &dispute_message = dispute_message_res.value();

              throw std::runtime_error("not implemented");  // TODO Implement it
              // DisputeDistributionMessage::SendDispute(dispute_message);
            } else {
              // LOG-ERROR: "No ongoing dispute, but we checked there is
              // one!"
            }
          }
        }
      }
    }

    // All good, update recent disputes if state has changed
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L999
    if (new_state.dispute_status.has_value()) {
      auto &new_status = new_state.dispute_status.value();

      // Only bother with db access, if there was an actual change.
      if (is_freshly_disputed or is_freshly_confirmed or is_freshly_concluded) {
        auto recent_disputes =
            storage_->load_recent_disputes().value_or(RecentDisputes{});

        auto [it, fresh] = recent_disputes.emplace(
            std::make_tuple(session, candidate_hash), Active{});

        if (fresh) {
          // LOG-INFO: "New dispute initiated for candidate."
        }

        // update status
        it->second = new_status;

        // LOG_TRACE: "Writing recent disputes with updates for candidate"
        storage_->write_recent_disputes(std::move(recent_disputes));
      }
    }

    // Notify ChainSelection if a dispute has concluded against a candidate.
    // ChainSelection will need to mark the candidate's relay parent as
    // reverted.
    if (is_freshly_concluded_against) {
      auto blocks_including =
          scraper_->get_blocks_including_candidate(candidate_hash);

      if (blocks_including.size() > 0) {
        throw std::runtime_error("not implemented");  // TODO Implement it
        // ctx.send_message(ChainSelectionMessage::RevertBlocks(blocks_including)).await;
      } else {
        // LOG-DEBUG "Could not find an including block for candidate
        // against
        //           which a dispute has concluded."
      }
    }

    // LOG-TRACE: "Import summary"

    return outcome::success(true);
  }

  outcome::result<DisputeMessage> DisputeCoordinatorImpl::make_dispute_message(
      SessionInfo session_info,
      CandidateVotes votes,
      SignedDisputeStatement our_vote,
      ValidatorIndex our_index) {
    auto &validators = session_info.validators;

    ValidatorIndex other_index;

    auto get_other_vote =
        [&](auto &votes) -> outcome::result<SignedDisputeStatement> {
      auto it = votes.begin();
      if (it == votes.end() or ++it == votes.end()) {
        return DisputeMessageCreationError::NoOppositeVote;
      }
      auto &statement = it->second;

      auto validator_index = statement.index;
      auto validator_signature = statement.signature;

      if (validator_index >= validators.size()) {
        return DisputeMessageCreationError::InvalidValidatorIndex;
      }
      auto validator_public = validators[validator_index];

      // check sig

      auto payload = getPayload(
          statement, our_vote.candidate_hash, our_vote.session_index);

      auto validation_res = sr25519_crypto_provider_->verify(
          validator_signature, payload, validator_public);

      if (validation_res.has_error()) {
        return validation_res.as_failure();
      }
      if (not validation_res.value()) {
        return DisputeMessageCreationError::InvalidStoredStatement;
      }

      // make other signed statement

      other_index = validator_index;

      return SignedDisputeStatement{
          .dispute_statement = statement,
          .candidate_hash = our_vote.candidate_hash,
          .validator_public = validator_public,
          .validator_signature = validator_signature,
          .session_index = our_vote.session_index,
      };
    };

    // if it is valid dispute statement
    auto is_vds = is_type<ValidDisputeStatement>(our_vote.dispute_statement);

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/lib.rs#L521
    OUTCOME_TRY(
        other_vote,
        is_vds ? get_other_vote(votes.invalid) : get_other_vote(votes.valid));

    auto &valid_statement = is_vds ? our_vote : other_vote;
    auto &valid_index = is_vds ? our_index : other_index;
    auto &invalid_statement = is_vds ? other_vote : our_vote;
    auto &invalid_index = is_vds ? other_index : our_index;

    auto &candidate_receipt = votes.candidate_receipt;

    auto candidate_hash = valid_statement.candidate_hash;
    // Check statements concern same candidate:
    if (candidate_hash != invalid_statement.candidate_hash) {
      return DisputeMessageConstructingError::CandidateHashMismatch;
    }

    auto session_index = valid_statement.session_index;
    if (session_index != invalid_statement.session_index) {
      return DisputeMessageConstructingError::SessionIndexMismatch;
    }

    if (valid_index >= session_info.validators.size()) {
      return DisputeMessageConstructingError::
          ValidStatementInvalidValidatorIndex;
    }
    if (invalid_index >= session_info.validators.size()) {
      return DisputeMessageConstructingError::
          InvalidStatementInvalidValidatorIndex;
    }

    auto &valid_id = session_info.validators[valid_index];
    if (valid_id != valid_statement.validator_public) {
      return DisputeMessageConstructingError::InvalidValidKey;
    }

    auto &invalid_id = session_info.validators[invalid_index];
    if (invalid_id != invalid_statement.validator_public) {
      return DisputeMessageConstructingError::InvalidInvalidKey;
    }

    if (candidate_receipt.commitments_hash != candidate_hash) {
      return DisputeMessageConstructingError::InvalidCandidateReceipt;
    }

    if (not is_type<ValidDisputeStatement>(valid_statement.dispute_statement)) {
      return DisputeMessageConstructingError::ValidStatementHasInvalidKind;
    }
    auto &valid_kind = boost::relaxed_get<ValidDisputeStatement>(
                           valid_statement.dispute_statement)
                           .kind;

    if (not is_type<InvalidDisputeStatement>(
            valid_statement.dispute_statement)) {
      return DisputeMessageConstructingError::InvalidStatementHasValidKind;
    }
    auto &invalid_kind = boost::relaxed_get<InvalidDisputeStatement>(
                             valid_statement.dispute_statement)
                             .kind;

    ValidDisputeVote valid_vote{
        .index = valid_index,
        .signature = valid_statement.validator_signature,
        .kind = valid_kind,
    };

    InvalidDisputeVote invalid_vote{
        .index = invalid_index,
        .signature = invalid_statement.validator_signature,
        .kind = invalid_kind,
    };

    return DisputeMessage{
        candidate_receipt,
        session_index,
        invalid_vote,
        valid_vote,
    };
  }
}  // namespace kagome::dispute

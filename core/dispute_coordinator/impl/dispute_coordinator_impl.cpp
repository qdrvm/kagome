/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"

#include <unordered_set>
#include <vector>

#include "../../../test/testutil/outcome/dummy_error.hpp"
#include "common/visitor.hpp"
#include "dispute_coordinator/chain_scraper.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/participation/participation.hpp"

namespace kagome::dispute {

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
      SL_DEBUG(
          log_,
          "We are lacking a `SessionInfo` for handling import of statements.");
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
      // Therefore it is reasonable to expect a simple vote request to succeed
      // way faster than disputes are raised.
      // 4. We are waiting (and blocking the whole subsystem) on a response
      // right after - therefore even with all else failing we will never have
      // more than one message in flight at any given time.

      // see: {polkadot}/node/core/dispute-coordinator/src/initialized.rs:809
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
          /// votes, will be ignored if a backing vote is already present. Any
          /// already existing `valid` vote, will be overridden by any given
          /// backing vote.

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
    //                    ? is_type<Confirmed>(new_state.dispute_status.value())
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
              // LOG-ERROR: "No ongoing dispute, but we checked there is one!"
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
        // LOG-DEBUG "Could not find an including block for candidate against
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

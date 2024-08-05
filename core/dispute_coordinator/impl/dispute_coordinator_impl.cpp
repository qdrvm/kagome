/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/dispute_coordinator_impl.hpp"

#include <future>
#include <set>
#include <unordered_set>
#include <vector>

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>

#include "application/app_state_manager.hpp"
#include "authority_discovery/query/query.hpp"
#include "blockchain/block_header_repository.hpp"
#include "common/main_thread_pool.hpp"
#include "common/visitor.hpp"
#include "consensus/timeline/timeline.hpp"
#include "dispute_coordinator/impl/chain_scraper_impl.hpp"
#include "dispute_coordinator/impl/dispute_thread_pool.hpp"
#include "dispute_coordinator/impl/errors.hpp"
#include "dispute_coordinator/impl/sending_dispute.hpp"
#include "dispute_coordinator/impl/spam_slots_impl.hpp"
#include "dispute_coordinator/participation/impl/participation_impl.hpp"
#include "dispute_coordinator/provisioner/impl/prioritized_selection.hpp"
#include "dispute_coordinator/provisioner/impl/random_selection.hpp"
#include "network/router.hpp"
#include "network/types/dispute_messages.hpp"
#include "parachain/approval/approval_distribution.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "utils/pool_handler_ready_make.hpp"
#include "utils/tuple_hash.hpp"

namespace kagome::dispute {

  namespace {

    constexpr auto disputesTotalMetricName =
        "kagome_parachain_candidate_disputes_total";

    constexpr auto disputeVotesMetricName =
        "kagome_parachain_candidate_dispute_votes";

    constexpr auto disputeConcludedMetricName =
        "kagome_parachain_candidate_dispute_concluded";

    constexpr auto disputesFinalityLagMetricName =
        "kagome_parachain_disputes_finality_lag";

    inline outcome::result<common::Buffer> getSignablePayload(
        const DisputeStatement &statement,
        const CandidateHash &candidate_hash,
        SessionIndex session) {
      auto res = visit_in_place(
          statement,
          [&](const ValidDisputeStatement &kind) {
            return visit_in_place(
                kind,
                [&](const Explicit &) -> outcome::result<std::vector<uint8_t>> {
                  std::array<uint8_t, 4> magic{'D', 'I', 'S', 'P'};
                  bool validity = true;
                  return scale::encode(
                      std::tie(magic, validity, candidate_hash, session));
                },
                [&](const BackingSeconded &inclusion_parent) {
                  std::array<uint8_t, 4> magic{'B', 'K', 'N', 'G'};
                  uint8_t discriminant = 1;  // Seconded
                  return scale::encode(std::tie(magic,
                                                discriminant,
                                                candidate_hash,
                                                session,
                                                inclusion_parent));
                },
                [&](const BackingValid &inclusion_parent) {
                  std::array<uint8_t, 4> magic{'B', 'K', 'N', 'G'};
                  uint8_t discriminant = 2;  // Valid
                  return scale::encode(std::tie(magic,
                                                discriminant,
                                                candidate_hash,
                                                session,
                                                inclusion_parent));
                },
                [&](const ApprovalChecking &) {
                  std::array<uint8_t, 4> magic{'A', 'P', 'P', 'R'};
                  return scale::encode(
                      std::tie(magic, candidate_hash, session));
                },
                [&](const ApprovalCheckingMultipleCandidates &candidates)
                    -> outcome::result<std::vector<uint8_t>> {
                  /// Returns Error if the candidate_hash is not included in the
                  /// list of signed candidate from
                  /// ApprovalCheckingMultipleCandidate.
                  if (std::find(
                          candidates.begin(), candidates.end(), candidate_hash)
                      == candidates.end()) {
                    return ApprovalCheckingMultipleCandidatesError::NotIncluded;
                  }

                  std::array<uint8_t, 4> magic{'A', 'P', 'P', 'R'};
                  // Make this backwards compatible with `ApprovalVote` so if
                  // we have just on the candidate signature will look the same.
                  // This gives us the nice benefit that old nodes can still
                  // check signatures when len is 1 and the new node can check
                  // the signature coming from old nodes.
                  if (candidates.size() == 1) {
                    return scale::encode(
                        std::tie(magic, candidates.front(), session));
                  } else {
                    return scale::encode(std::tie(magic, candidates, session));
                  }
                });
          },
          [&](const InvalidDisputeStatement &kind) {
            return visit_in_place(kind, [&](const Explicit &) {
              std::array<uint8_t, 4> magic{'D', 'I', 'S', 'P'};
              bool validity = false;
              return scale::encode(
                  std::tie(magic, validity, candidate_hash, session));
            });
          });
      if (res.has_value()) {
        return common::Buffer(std::move(res.value()));
      }
      return res.as_failure();
    }

  }  // namespace

  DisputeCoordinatorImpl::DisputeCoordinatorImpl(
      std::shared_ptr<application::ChainSpec> chain_spec,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      clock::SystemClock &system_clock,
      clock::SteadyClock &steady_clock,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<Storage> storage,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_crypto_provider,
      std::shared_ptr<blockchain::BlockHeaderRepository>
          block_header_repository,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> core_api,
      std::shared_ptr<runtime::ParachainHost> api,
      std::shared_ptr<parachain::Recovery> recovery,
      std::shared_ptr<parachain::Pvf> pvf,
      std::shared_ptr<parachain::ApprovalDistribution> approval_distribution,
      std::shared_ptr<authority_discovery::Query> authority_discovery,
      common::MainThreadPool &main_thread_pool,
      DisputeThreadPool &dispute_thread_pool,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<network::PeerView> peer_view,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      LazySPtr<consensus::Timeline> timeline)
      : log_(log::createLogger("DisputeCoordinator", "dispute")),
        system_clock_(system_clock),
        steady_clock_(steady_clock),
        session_keys_(std::move(session_keys)),
        storage_(std::move(storage)),
        sr25519_crypto_provider_(std::move(sr25519_crypto_provider)),
        block_header_repository_(std::move(block_header_repository)),
        hasher_(std::move(hasher)),
        block_tree_(std::move(block_tree)),
        core_api_(std::move(core_api)),
        api_(std::move(api)),
        recovery_(std::move(recovery)),
        pvf_(std::move(pvf)),
        approval_distribution_(std::move(approval_distribution)),
        authority_discovery_(std::move(authority_discovery)),
        router_(std::move(router)),
        peer_view_(std::move(peer_view)),
        chain_sub_{std::move(chain_sub_engine)},
        timeline_(std::move(timeline)),
        main_pool_handler_{main_thread_pool.handler(*app_state_manager)},
        dispute_thread_handler_{poolHandlerReadyMake(
            this, app_state_manager, dispute_thread_pool, log_)},
        scheduler_{std::make_shared<libp2p::basic::SchedulerImpl>(
            std::make_shared<libp2p::basic::AsioSchedulerBackend>(
                dispute_thread_pool.io_context()),
            libp2p::basic::Scheduler::Config{})},
        runtime_info_(std::make_unique<RuntimeInfo>(api_, session_keys_)),
        batches_(std::make_unique<Batches>(log_, steady_clock_, hasher_)) {
    BOOST_ASSERT(session_keys_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(sr25519_crypto_provider_ != nullptr);
    BOOST_ASSERT(block_header_repository_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(core_api_ != nullptr);
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(recovery_ != nullptr);
    BOOST_ASSERT(pvf_ != nullptr);
    BOOST_ASSERT(approval_distribution_ != nullptr);
    BOOST_ASSERT(authority_discovery_ != nullptr);
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(dispute_thread_handler_ != nullptr);
    BOOST_ASSERT(router_ != nullptr);
    BOOST_ASSERT(peer_view_ != nullptr);

    // Register metrics
    metrics_registry_->registerCounterFamily(disputesTotalMetricName,
                                             "Total number of raised disputes");
    metric_disputes_total_ =
        metrics_registry_->registerCounterMetric(disputesTotalMetricName);

    metrics_registry_->registerGaugeFamily(
        disputesFinalityLagMetricName,
        "How far behind the head of the chain the Disputes protocol wants to "
        "vote");
    metric_disputes_finality_lag_ =
        metrics_registry_->registerGaugeMetric(disputesFinalityLagMetricName);
    metric_disputes_finality_lag_->set(0);

    metrics_registry_->registerCounterFamily(
        disputeVotesMetricName,
        "Accumulated dispute votes, sorted by candidate is valid or invalid");
    metric_votes_valid_ = metrics_registry_->registerCounterMetric(
        disputeVotesMetricName,
        {{"validity", "valid"}, {"chain", chain_spec->chainType()}});
    metric_votes_invalid_ = metrics_registry_->registerCounterMetric(
        disputeVotesMetricName,
        {{"validity", "invalid"}, {"chain", chain_spec->chainType()}});

    metrics_registry_->registerCounterFamily(
        disputeConcludedMetricName,
        "Concluded dispute votes, sorted by candidate is valid or invalid");
    metric_concluded_valid_ = metrics_registry_->registerCounterMetric(
        disputeConcludedMetricName,
        {{"validity", "valid"}, {"chain", chain_spec->chainType()}});
    metric_concluded_invalid_ = metrics_registry_->registerCounterMetric(
        disputeConcludedMetricName,
        {{"validity", "invalid"}, {"chain", chain_spec->chainType()}});
  }

  bool DisputeCoordinatorImpl::tryStart() {
    auto leaves = block_tree_->getLeaves();
    active_heads_.insert(leaves.begin(), leaves.end());

    // subscribe to leaves update
    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    primitives::events::subscribe(
        *my_view_sub_,
        network::PeerView::EventType::kViewUpdated,
        [wptr{weak_from_this()}](const network::ExView &event) {
          if (auto self = wptr.lock()) {
            self->on_active_leaves_update(event);
          }
        });

    // subscribe to finalization
    chain_sub_.onFinalize(
        [weak{weak_from_this()}](const primitives::BlockHeader &block) {
          if (auto self = weak.lock()) {
            self->on_finalized_block(block.blockInfo());
          }
        });

    return true;
  }

  void DisputeCoordinatorImpl::startup(const network::ExView &updated) {
    BOOST_ASSERT(not initialized_);

    scraper_ = std::make_unique<ChainScraperImpl>(api_, block_tree_, hasher_);

    auto first_leaf = ActivatedLeaf{.hash = updated.new_head.hash(),
                                    .number = updated.new_head.number,
                                    .status = LeafStatus::Fresh};

    auto now = system_clock_.nowUint64();

    auto recent_disputes_res = storage_->load_recent_disputes();
    if (recent_disputes_res.has_error()) {
      log_->error("Failed initial load of recent disputes: {}",
                  recent_disputes_res.error());
      return;
    }
    auto &recent_disputes_opt = recent_disputes_res.value();

    std::vector<std::tuple<SessionIndex, CandidateHash, DisputeStatus>>
        active_disputes;
    if (recent_disputes_opt.has_value()) {
      for (const auto &p : recent_disputes_opt.value()) {
        const auto &status = p.second;

        auto at = visit_in_place(
            status,
            [](const Active &) -> std::optional<Timestamp> {
              return std::nullopt;
            },
            [](const Confirmed &) -> std::optional<Timestamp> {
              return std::nullopt;
            },
            [](const ConcludedFor &at) -> std::optional<Timestamp> {
              return (Timestamp)at;
            },
            [](const ConcludedAgainst &at) -> std::optional<Timestamp> {
              return (Timestamp)at;
            },
            [](const Postponed &) -> std::optional<Timestamp> {
              return std::nullopt;
            });

        auto dispute_is_inactive =
            at.has_value() and at.value() + kActiveDurationSecs < now;

        if (not dispute_is_inactive) {
          active_disputes.emplace_back(
              std::get<0>(p.first), std::get<1>(p.first), p.second);
        }
      }
    }

    std::vector<std::tuple<ParticipationPriority, ParticipationRequest>>
        participation_requests;

    SpamSlotsImpl::UnconfirmedDisputes spam_disputes;

    ActiveLeavesUpdate update{.activated = first_leaf, .deactivated = {}};
    auto updates_res = scraper_->process_active_leaves_update(update);
    if (updates_res.has_error()) {
      log_->error("Failed initialize scrapper: {}", updates_res.error());
      return;
    }
    auto &votes = updates_res.value().on_chain_votes;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/lib.rs#L298
    for (auto &[session, candidate_hash, status] : active_disputes) {
      auto env_opt =
          makeCandidateEnvironment(*session_keys_, session, first_leaf.hash);
      if (not env_opt.has_value()) {
        continue;
      }
      auto &env = env_opt.value();

      auto votes_res = storage_->load_candidate_votes(session, candidate_hash);
      if (votes_res.has_error()) {
        SL_ERROR(log_,
                 "Failed initial load of candidate votes: {}",
                 votes_res.error());
        continue;
      }
      if (not votes_res.value().has_value()) {
        SL_ERROR(log_, "Failed initial load of candidate votes: not found");
        continue;
      }
      auto &candidate_votes = votes_res.value().value();

      auto &relay_parent =
          candidate_votes.candidate_receipt.descriptor.relay_parent;

      auto disabled_validators_res = api_->disabled_validators(relay_parent);
      if (disabled_validators_res.has_error()) {
        SL_WARN(log_,
                "Cannot import votes, without getting disabled validators: {}",
                disabled_validators_res.error());
        continue;
      }
      auto &disabled_validators = disabled_validators_res.value();

      auto vote_state = CandidateVoteState::create(
          candidate_votes, env, disabled_validators, system_clock_.nowUint64());

      auto is_included = scraper_->is_candidate_included(candidate_hash);
      auto is_backed = scraper_->is_candidate_backed(candidate_hash);
      auto is_disputed = vote_state.dispute_status.has_value();
      auto is_postponed =
          is_disputed ? is_type<Postponed>(vote_state.dispute_status.value())
                      : false;
      auto is_confirmed =
          is_disputed ? is_type<Confirmed>(vote_state.dispute_status.value())
                      : false;
      auto is_potential_spam = is_disputed  //
                           and not is_included and not is_backed
                           and not is_confirmed and not is_postponed;

      if (is_potential_spam) {
        SL_TRACE(log_,
                 "Found potential spam dispute on startup "
                 "(session={}, candidate={})",
                 session,
                 candidate_hash);

        std::set<ValidatorIndex> voted_indices;
        for (auto &[key, _] : vote_state.votes.valid) {
          voted_indices.emplace(key);
        }
        for (auto &[key, _] : vote_state.votes.invalid) {
          voted_indices.emplace(key);
        }

        spam_disputes.emplace(std::make_tuple(session, candidate_hash),
                              std::move(voted_indices));
      } else {
        auto own_vote = if_type<Voted>(vote_state.own_vote);
        if (own_vote.has_value() and own_vote->get().empty()) {
          // Participate if need be:

          SL_TRACE(log_,
                   "Found valid dispute, with no vote from us on startup - "
                   "participating. (session={}, candidate={})",
                   session,
                   candidate_hash);

          auto &receipt = vote_state.votes.candidate_receipt;

          participation_requests.emplace_back(
              static_cast<ParticipationPriority>(is_included),
              ParticipationRequest{.candidate_hash = receipt.hash(*hasher_),
                                   .candidate_receipt = receipt,
                                   .session = session});

        } else {
          // Else make sure our own vote is distributed:

          SL_TRACE(log_,
                   "Found valid dispute, with vote from us on startup - "
                   "send vote. (session={}, candidate={})",
                   session,
                   candidate_hash);

          send_dispute_messages(env, vote_state);
        }
      }
    }

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/spam_slots.rs#L64
    SpamSlotsImpl::Slots slots;
    for (auto &[session_and_candidate, validators] : spam_disputes) {
      auto &session = std::get<0>(session_and_candidate);
      for (auto &validator : validators) {
        auto &spam_vote_count = slots[std::make_tuple(session, validator)];
        ++spam_vote_count;
        if (spam_vote_count > SpamSlotsImpl::kMaxSpamVotes) {
          SL_DEBUG(log_,
                   "Import exceeded spam slot for validator "
                   "(session={}, validator={}, count={})",
                   session,
                   validator,
                   spam_vote_count);
        }
      }
    }
    spam_slots_ = std::make_unique<SpamSlotsImpl>(std::move(slots),
                                                  std::move(spam_disputes));

    initialized_ = true;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L166
    for (const auto &[priority, request] : std::move(participation_requests)) {
      auto res = participation_->queue_participation(priority, request);
      if (res.has_error()) {
        SL_ERROR(log_, "Can't queue startup participation: {}", res.error());
      }
    }

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L160
    for (auto &vote : votes) {
      auto res = process_on_chain_votes(vote);
      if (res.has_error()) {
        SL_WARN(log_, "Skipping scraping block due to error: {}", res.error());
      }
    }

    participation_ =
        std::make_shared<ParticipationImpl>(block_header_repository_,
                                            hasher_,
                                            api_,
                                            recovery_,
                                            pvf_,
                                            dispute_thread_handler_,
                                            weak_from_this());

    // Also provide first leaf to participation for good measure.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L192
    auto first_leaf_update_res = participation_->process_active_leaves_update(
        ActiveLeavesUpdate{.activated = first_leaf});
    if (first_leaf_update_res.has_error()) {
      SL_ERROR(log_,
               "Can't process first active leaf update: {}",
               first_leaf_update_res.error());
    }
  }

  void DisputeCoordinatorImpl::onParticipation(
      const ParticipationStatement &message) {
    if (not initialized_) {
      return;
    }

    REINVOKE(*dispute_thread_handler_, onParticipation, message);

    SL_TRACE(log_, "MuxedMessage::Participation");

    auto res = participation_->get_participation_result(message);
    if (res.has_error()) {
      SL_ERROR(log_, "Can't get participation result: {}", res.error());
      return;
    }

    const auto session = message.session;
    const auto &candidate_hash = message.candidate_hash;
    const auto &candidate_receipt = message.candidate_receipt;
    const auto outcome = message.outcome;

    if (outcome == ParticipationOutcome::Valid
        or outcome == ParticipationOutcome::Invalid) {
      SL_TRACE(log_, "Issuing local statement based on participation outcome");

      auto issue_res =
          issue_local_statement(candidate_hash,
                                candidate_receipt,
                                session,
                                outcome == ParticipationOutcome::Valid);
      if (issue_res.has_error()) {
        SL_ERROR(log_, "Can't issue local statement: {}", issue_res.error());
        return;
      }

    } else {
      SL_WARN(log_, "Dispute participation failed");
    }
  }

  void DisputeCoordinatorImpl::on_active_leaves_update(
      const network::ExView &updated) {
    if (not timeline_.get()->wasSynchronized()) {
      return;
    }

    REINVOKE(*dispute_thread_handler_, on_active_leaves_update, updated);

    if (not initialized_) {
      return startup(updated);
    }

    ActiveLeavesUpdate update{.activated = {{.hash = updated.new_head.hash(),
                                             .number = updated.new_head.number,
                                             .status = LeafStatus::Fresh}},
                              .deactivated = updated.lost};

    auto res = process_active_leaves_update(update);
    if (res.has_error()) {
      SL_ERROR(log_, "Can't handle active list update: {}", res.error());
      return;
    }
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

    // Scraped on-chain backing votes for the candidates with the new active
    // leaf as if we received them via gossip.
    for (auto &[candidate_receipt, backers] :
         backing_validators_per_candidate) {
      auto &relay_parent = candidate_receipt.descriptor.relay_parent;
      auto &candidate_hash = candidate_receipt.hash(*hasher_);

      SL_TRACE(log_, "Importing backing votes from chain for candidate");

      OUTCOME_TRY(session, api_->session_index_for_child(relay_parent));
      OUTCOME_TRY(session_info_opt, api_->session_info(relay_parent, session));
      BOOST_ASSERT(session_info_opt.has_value());
      const auto &session_info = session_info_opt.value();

      std::vector<Indexed<SignedDisputeStatement>> statements;
      for (auto &[validator_index, attestation] : backers) {
        if (validator_index >= session_info.validators.size()) {
          SL_ERROR(log_,
                   "Missing public key for validator #{} (all={})",
                   validator_index,
                   session_info.validators.size());
          continue;  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L391
        }

        ValidatorId validator_public = session_info.validators[validator_index];

        ValidatorSignature validator_signature = visit_in_place(
            attestation,
            [](const Unused<0> &) {
              UNREACHABLE;
              return ValidatorSignature{};
            },
            [](const auto &sig) {  //
              return (ValidatorSignature)sig;
            });

        auto valid_statement_kind = visit_in_place(
            attestation,
            [](const Unused<0> &) {
              UNREACHABLE;
              return ValidDisputeStatement{};
            },
            [&](const ImplicitValidityAttestation &) {
              return ValidDisputeStatement(BackingSeconded(relay_parent));
            },
            [&](const ExplicitValidityAttestation &) {
              return ValidDisputeStatement(BackingValid(relay_parent));
            });

        DisputeStatement statement{valid_statement_kind};

        [[maybe_unused]] auto check_sig = [&]() -> bool {
          auto payload_res =
              getSignablePayload(statement, candidate_hash, session);
          if (payload_res.has_error()) {
            SL_CRITICAL(log_,
                        "Scraped backing votes produces bad payload! {}",
                        payload_res.error());
            return false;
          }
          auto &payload = payload_res.value();

          auto validation_res = sr25519_crypto_provider_->verify(
              validator_signature, payload, validator_public);

          if (validation_res.has_error()) {
            SL_CRITICAL(log_,
                        "Cannot validate scraped backing votes signature! {}",
                        validation_res.error());
            return false;
          }
          if (not validation_res.value()) {
            SL_CRITICAL(log_, "Scraped backing votes had invalid signature!");
            return false;
          }
          return true;
        };
        BOOST_ASSERT(check_sig());

        Indexed<SignedDisputeStatement> signed_dispute_statement{
            {
                statement,
                candidate_hash,
                validator_public,
                validator_signature,
                session,
            },
            validator_index};

        statements.emplace_back(std::move(signed_dispute_statement));
      }

      // Importantly, handling import statements for backing votes also
      // clears spam slots for any newly backed candidates
      OUTCOME_TRY(
          import_result,
          handle_import_statements(candidate_receipt, session, statements));

      if (import_result) {
        SL_TRACE(log_, "Imported backing votes from chain");
      } else {
        SL_WARN(log_, "Attempted import of on-chain backing votes failed");
      }
    }

    // Import disputes from on-chain, this already went through a vote so it's
    // assumed as verified. This will only be stored, gossiping it is not
    // necessary.

    // First, try to obtain all the backings which ultimately contain the
    // candidate receipt which we need.

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L461
    for (auto &dispute_statement_set : disputes) {
      auto &dispute_candidate = dispute_statement_set.candidate_hash;
      auto &dispute_session = dispute_statement_set.session;
      auto &dispute_statements = dispute_statement_set.statements;
      SL_TRACE(log_, "Importing dispute votes from chain for candidate");

      std::vector<Indexed<SignedDisputeStatement>> statements;
      for (const auto &dispute_data : dispute_statements) {
        const auto &dispute_statement = std::get<0>(dispute_data);
        const auto &validator_index = std::get<1>(dispute_data);
        const auto &validator_signature = std::get<2>(dispute_data);
        auto res = visit_in_place(
            dispute_statement,
            [&](const auto &statement_kind) -> outcome::result<void> {
              auto session_info_opt_res = api_->session_info({}, session);
              if (session_info_opt_res.has_error()) {
                SL_WARN(log_,
                        "Could not retrieve session info: {}",
                        session_info_opt_res.error());
                return outcome::success();
              }
              auto session_info_opt = session_info_opt_res.value();
              if (not session_info_opt.has_value()) {
                SL_WARN(log_, "Could not retrieve session info: not found");
                return outcome::success();
              }
              auto session_info = std::move(session_info_opt.value());

              if (validator_index >= session_info.validators.size()) {
                SL_ERROR(log_,
                         "Missing public key for validator #{} that "
                         "participated in concluded dispute",
                         validator_index);
                return SignatureValidationError::MissingPublicKey;
              }

              ValidatorId validator_public =
                  session_info.validators[validator_index];

              [[maybe_unused]] auto check_sig = [&] {
                auto payload_res = getSignablePayload(
                    dispute_statement, dispute_candidate, dispute_session);
                if (payload_res.has_error()) {
                  SL_CRITICAL(log_,
                              "Scraped dispute votes produces bad payload! {}",
                              payload_res.error());
                  return false;
                }
                auto &payload = payload_res.value();

                auto validation_res = sr25519_crypto_provider_->verify(
                    validator_signature, payload, validator_public);

                if (validation_res.has_error()) {
                  SL_CRITICAL(
                      log_,
                      "Cannot validate scraped dispute votes signature! {}",
                      validation_res.error());
                  return false;
                }
                if (not validation_res.value()) {
                  SL_CRITICAL(log_,
                              "Scraped dispute votes had invalid signature!");
                  return false;
                }

                return true;
              };
              BOOST_ASSERT(check_sig());

              Indexed<SignedDisputeStatement> signed_dispute_statement{
                  {
                      dispute_statement,
                      dispute_candidate,
                      validator_public,
                      validator_signature,
                      dispute_session,
                  },
                  validator_index};

              statements.emplace_back(std::move(signed_dispute_statement));
              return outcome::success();
            });
        OUTCOME_TRY(res);
      }

      OUTCOME_TRY(import_result,
                  handle_import_statements(
                      dispute_candidate, dispute_session, statements));

      if (import_result) {
        SL_TRACE(log_, "Imported statement of dispute from on-chain");
      } else {
        SL_WARN(log_,
                "Attempted import of on-chain statement of dispute failed");
      }
    }

    return outcome::success();
  }

  outcome::result<void> DisputeCoordinatorImpl::process_active_leaves_update(
      const ActiveLeavesUpdate &update) {
    BOOST_ASSERT(initialized_);

    OUTCOME_TRY(scraped_updates,
                scraper_->process_active_leaves_update(update));

    auto bump_res = participation_->bump_to_priority_for_candidates(
        scraped_updates.included_receipts);
    if (bump_res.has_error()) {
      SL_ERROR(log_, "Can't bump priority for candidate: {}", bump_res.error());
    }

    OUTCOME_TRY(participation_->process_active_leaves_update(update));

    if (update.activated.has_value()) {
      auto &new_leaf = update.activated.value();

      // Get session index of new leaf
      OUTCOME_TRY(session_index, api_->session_index_for_child(new_leaf.hash));

      // If the latest session was updated, then prune spam slots
      if (highest_session_ < session_index) {
        highest_session_ = session_index;
        static size_t kWindowSize = 6;
        spam_slots_->prune_old(highest_session_ - kWindowSize);
      }

      // The `runtime-api` subsystem has an internal queue which serializes the
      // execution, so there is no point in running these in parallel.

      for (auto &votes : scraped_updates.on_chain_votes) {
        auto process_res = process_on_chain_votes(votes);
        if (process_res.has_error()) {
          SL_WARN(log_,
                  "Skipping scraping block due to error: {}",
                  process_res.error());
        }
      }

      // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L205
      active_heads_.emplace(new_leaf.hash);
    }

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L206
    for (auto &leaf : update.deactivated) {
      active_heads_.erase(leaf);
    }

    // Initiate fetching for new active disputes if needed
    auto res = refresh_sessions();
    if (not res.has_error()) {
      auto sessions_updated = res.value();

      auto waiting = std::move(waiting_for_active_disputes_);
      waiting_for_active_disputes_.reset();
      if (not waiting.has_value()) {
        // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L196

        waiting_for_active_disputes_.emplace(
            WaitForActiveDisputesState{sessions_updated});

        dispute_thread_handler_->execute([wp{weak_from_this()}] {
          // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L219
          if (auto self = wp.lock()) {
            self->getActiveDisputes([wp](auto active_disputes_res) {
              if (auto self = wp.lock()) {
                self->handle_active_dispute_response(
                    std::move(active_disputes_res));
              }
            });
          }
        });

      } else {
        // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L196

        if (sessions_updated) {
          waiting_for_active_disputes_->have_new_sessions = true;
        }

        SL_DEBUG(log_,
                 "Dispute coordinator slow? We are still waiting for data on "
                 "next active leaves update.");
      }
    }

    return outcome::success();
  }

  outcome::result<bool> DisputeCoordinatorImpl::refresh_sessions() {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L294
    std::unordered_map<SessionIndex, primitives::BlockHash> new_sessions;

    // Iterate all heads we track as active and fetch the child' session
    // indices.
    for (auto &head : active_heads_) {
      OUTCOME_TRY(session_index,
                  runtime_info_->get_session_index_for_child(head));
      new_sessions.emplace(session_index, head);
    }

    /// Make active sessions correspond to currently active heads.
    auto sessions_updated =
        std::equal(active_sessions_.begin(),
                   active_sessions_.end(),
                   new_sessions.begin(),
                   new_sessions.end(),
                   [](auto it1, auto it2) { return it1.first == it2.first; });

    // Update in any case, so we use current heads for queries:
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L304
    active_sessions_ = std::move(new_sessions);

    return sessions_updated;
  }

  /// Handle new active disputes response.
  ///
  /// - Initiate a retry of failed sends which are still active.
  /// - Get new authorities to send messages to.
  /// - Get rid of obsolete tasks and disputes.
  ///
  /// This function ensures the `SEND_RATE_LIMIT`, therefore it might block.
  void DisputeCoordinatorImpl::handle_active_dispute_response(
      outcome::result<OutputDisputes> active_disputes_res) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L184
    auto state = std::move(waiting_for_active_disputes_);
    waiting_for_active_disputes_.reset();
    auto have_new_sessions = state.has_value() and state->have_new_sessions;

    if (active_disputes_res.has_error()) {
      SL_WARN(log_,
              "Active dispute obtaining was failed: {}",
              active_disputes_res.error());
      return;
    }
    auto &active_disputes = active_disputes_res.value();

    /// Handle new active disputes response.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L261
    std::unordered_set<CandidateHash> candidates;
    std::for_each(active_disputes.begin(),
                  active_disputes.end(),
                  [&](const auto &active_dispute) {
                    candidates.emplace(std::get<1>(active_dispute));
                  });

    // Cleanup obsolete senders
    sending_disputes_.remove_if([&](const auto &x) {
      const auto &candidate_hash = std::get<0>(x);
      return candidates.find(candidate_hash) == candidates.end();
    });

    // Iterates in order of insertion:
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/sender/mod.rs#L267
    auto should_rate_limit = true;
    for (auto &[candidate_hash, sending_dispute] : sending_disputes_) {
      if (not have_new_sessions and not sending_dispute->has_failed_sends()) {
        continue;
      }

      if (should_rate_limit) {
        // self.rate_limit
        //   .limit("while going through new sessions/failed sends",
        //   *candidate_hash) .await;
      }

      auto sends_happened =
          sending_dispute->refresh_sends(*runtime_info_, active_sessions_);
      //.await?;

      // Only rate limit if we actually sent something out _and_ it was not just
      // because of errors on previous sends.
      //
      // Reasoning: It would not be acceptable to slow down the whole subsystem,
      // just because of a few bad peers having problems. It is actually better
      // to risk running into their rate limit in that case and accept a minor
      // reputation change.
      should_rate_limit = sends_happened && have_new_sessions;
    }
  }

  void DisputeCoordinatorImpl::on_finalized_block(
      const primitives::BlockInfo &finalized) {
    if (not initialized_) {
      return;
    }

    REINVOKE(*dispute_thread_handler_, on_finalized_block, finalized);

    auto res = process_finalized_block(finalized);
    if (res.has_error()) {
      SL_ERROR(
          log_, "Can't process finalized block {}: {}", finalized, res.error());
    }
  }

  outcome::result<void> DisputeCoordinatorImpl::process_finalized_block(
      const primitives::BlockInfo &finalized) {
    BOOST_ASSERT(initialized_);
    return scraper_->process_finalized_block(finalized);
  }

  std::optional<CandidateEnvironment>
  DisputeCoordinatorImpl::makeCandidateEnvironment(
      crypto::SessionKeys &session_keys,
      SessionIndex session,
      primitives::BlockHash relay_parent) {
    auto session_info_opt_res = api_->session_info(relay_parent, session);
    if (session_info_opt_res.has_error()) {
      SL_WARN(log_,
              "Getting of session info was failed: {}",
              session_info_opt_res.error());
      return std::nullopt;
    }
    auto session_info_opt = session_info_opt_res.value();
    if (not session_info_opt.has_value()) {
      return std::nullopt;
    }
    auto session_info = std::move(session_info_opt.value());

    std::unordered_set<ValidatorIndex> controlled_indices;
    auto keypair = session_keys.getParaKeyPair(session_info.validators);
    if (keypair.has_value()) {
      controlled_indices.emplace(keypair->second);
    }

    return CandidateEnvironment{.session_index = session,
                                .session = std::move(session_info),
                                .controlled_indices = controlled_indices};
  }

  outcome::result<bool> DisputeCoordinatorImpl::handle_import_statements(
      MaybeCandidateReceipt candidate_receipt,
      const SessionIndex session,
      std::vector<Indexed<SignedDisputeStatement>> statements) {
    BOOST_ASSERT(initialized_);

    auto now = system_clock_.nowUint64();

    const auto &candidate_hash =
        is_type<CandidateReceipt>(candidate_receipt)
            ? boost::relaxed_get<CandidateReceipt>(candidate_receipt)
                  .hash(*hasher_)
            : boost::relaxed_get<CandidateHash>(candidate_receipt);

    // In case we are not provided with a candidate receipt we operate under
    // the assumption, that a previous vote which included a
    // `CandidateReceipt` was seen. This holds since every block is preceded
    // by the `Backing`-phase.
    //
    // There is one exception: A sufficiently sophisticated attacker could
    // prevent us from seeing the backing votes by withholding arbitrary
    // blocks, and hence we do not have a `CandidateReceipt` available.

    CandidateVoteState old_state;
    std::vector<ValidatorIndex> disabled_validators;

    OUTCOME_TRY(old_state_opt,
                storage_->load_candidate_votes(session, candidate_hash));

    primitives::BlockHash relay_parent{};

    if (not old_state_opt.has_value()) {
      auto provided_opt = if_type<CandidateReceipt>(candidate_receipt);
      if (not provided_opt.has_value()) {
        SL_ERROR(log_,
                 "Cannot import votes, without `CandidateReceipt` available!");
        return outcome::success(false);
      }

      relay_parent = provided_opt.value().get().descriptor.relay_parent;

      old_state = {
          .votes = {.candidate_receipt = provided_opt.value().get()},
          .own_vote = CannotVote{},
          .dispute_status = std::nullopt,
      };

    } else {
      relay_parent = old_state_opt->candidate_receipt.descriptor.relay_parent;
    }

    auto env_opt =
        makeCandidateEnvironment(*session_keys_, session, relay_parent);
    if (not env_opt.has_value()) {
      SL_DEBUG(log_,
               "We are lacking a `SessionInfo` for handling import of "
               "statements.");
      return outcome::success(false);
    }
    auto &env = env_opt.value();

    auto disabled_validators_res = api_->disabled_validators(relay_parent);
    if (disabled_validators_res.has_error()) {
      SL_WARN(log_,
              "Cannot import votes, without getting disabled validators: {}",
              disabled_validators_res.error());
      return outcome::success(false);
    }
    disabled_validators = std::move(disabled_validators_res.value());

    auto is_disabled = [&disabled_validators =
                            disabled_validators_res.value()](auto index) {
      return std::binary_search(
          disabled_validators.begin(), disabled_validators.end(), index);
    };

    if (old_state_opt.has_value()) {
      old_state = CandidateVoteState::create(
          old_state_opt.value(), env, disabled_validators, now);
    }

    SL_TRACE(log_, "Loaded votes");

    struct ImportResult {
      CandidateVoteState old_state;
      CandidateVoteState new_state;
      size_t imported_invalid_votes;
      size_t imported_valid_votes;
      size_t imported_approval_votes;
      std::vector<ValidatorIndex> new_invalid_voters;
    };

    /// Import fresh statements.
    ///
    /// Intermediate result will be a new state plus information about
    /// things that changed due to the import.

    auto votes = std::move(old_state.votes);

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/import.rs#L231

    std::vector<ValidatorIndex> new_invalid_voters;
    size_t imported_invalid_votes = 0;
    size_t imported_valid_votes = 0;

    auto expected_candidate_hash = votes.candidate_receipt.hash(*hasher_);

    std::vector<Indexed<SignedDisputeStatement>> postponed_statements;

    for (auto &vote : statements) {
      auto val_index = vote.ix;
      SignedDisputeStatement &statement = vote.payload;

      if (val_index >= env.session.validators.size()
          or env.session.validators[val_index] != statement.validator_public) {
        SL_WARN(log_, "Validator index doesn't match claimed key");
        continue;
      }

      if (statement.candidate_hash != expected_candidate_hash) {
        SL_WARN(log_, "Vote is for unexpected candidate!");
        continue;
      }

      if (statement.session_index != env.session_index) {
        SL_WARN(log_, "Vote is for unexpected session!");
        continue;
      }

      auto is_disabled_validator = is_disabled(val_index);

      // Postpone votes of disabled validators while any votes for candidate are
      // not exist
      if (is_disabled_validator and votes.valid.empty()
          and votes.invalid.empty()) {
        postponed_statements.emplace_back(std::move(vote));
        continue;
      }

      visit_in_place(
          statement.dispute_statement,
          [&](const ValidDisputeStatement &valid) {
            auto [it, fresh] = votes.valid.emplace(
                val_index,
                std::make_tuple(valid, statement.validator_signature));
            if (fresh) {
              if (imported_valid_votes == 0) {
                // Return postponed statements to process
                std::move_backward(postponed_statements.begin(),
                                   postponed_statements.end(),
                                   statements.end());
              }
              ++imported_valid_votes;
            }
          },
          [&](const InvalidDisputeStatement &invalid) {
            auto [it, fresh] = votes.invalid.emplace(
                val_index,
                std::make_tuple(invalid, statement.validator_signature));
            if (fresh) {
              new_invalid_voters.push_back(val_index);
              if (imported_invalid_votes == 0) {
                // Return postponed statements to process
                std::move_backward(postponed_statements.begin(),
                                   postponed_statements.end(),
                                   statements.end());
              }
              ++imported_invalid_votes;
            }
          });
    }

    ImportResult intermediate_result{
        std::move(old_state),
        CandidateVoteState::create(
            votes, env, disabled_validators, now),  // new_state
        imported_invalid_votes,
        imported_valid_votes,
        0,
        new_invalid_voters,
    };

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
      SL_TRACE(log_, "Requesting approval signatures");

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

      auto promise_res = std::promise<
          parachain::ApprovalDistribution::SignaturesForCandidate>();
      auto res_future = promise_res.get_future();

      approval_distribution_->getApprovalSignaturesForCandidate(
          candidate_hash,
          [promise_res = std::ref(promise_res)](
              parachain::ApprovalDistribution::SignaturesForCandidate res) {
            promise_res.get().set_value(std::move(res));
          });

      if (not res_future.valid()) {
        SL_WARN(log_,
                "Fetch for approval votes got cancelled, "
                "only expected during shutdown!");
        import_result = std::move(intermediate_result);
      } else {
        auto approval_votes = res_future.get();
        SL_TRACE(log_,
                 "Successfully received approval votes: {}",
                 approval_votes.size());

        // import approval votes

        import_result = std::move(intermediate_result);

        auto _votes = std::move(import_result.new_state.votes);

        for (auto &[index, signatures_data] : approval_votes) {
          auto &[hash, candidates, signature] = signatures_data;
          // clang-format off
          BOOST_ASSERT_MSG(
              [&] {
              // see: {polkadot}node/core/dispute-coordinator/src/import.rs:490
              // let pub_key = &env.session_info().validators.get(index).expect("indices are validated by approval-voting subsystem; qed");
              // let candidate_hash = votes.candidate_receipt.hash();
              // let session_index = env.session_index();
              // DisputeStatement::Valid(ValidDisputeStatement::ApprovalChecking)
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
                std::make_tuple(ValidDisputeStatement{ApprovalChecking{}},
                                signature));
            affected = true;
          } else {
            auto &existing = std::get<0>(it->second);
            if (is_type<BackingValid>(existing)
                or is_type<BackingSeconded>(existing)) {
              affected = false;
            } else if (is_type<Explicit>(existing)
                       or is_type<ApprovalChecking>(existing)) {
              affected = not is_type<ApprovalChecking>(existing);
              it->second = std::make_tuple(
                  ValidDisputeStatement{ApprovalChecking{}}, signature);
            }
          }

          if (affected) {
            import_result.imported_valid_votes += 1;
            import_result.imported_approval_votes += 1;
          }
        }

        import_result.new_state = CandidateVoteState::create(
            std::move(_votes), env, disabled_validators, now);
      }
    } else {
      SL_TRACE(log_, "Not requested approval signatures");
      import_result = intermediate_result;
    }

    SL_TRACE(log_, "Import result ready");

    auto &new_state = import_result.new_state;

    auto is_included = scraper_->is_candidate_included(candidate_hash);
    auto is_backed = scraper_->is_candidate_backed(candidate_hash);
    auto own_vote_missing =
        is_type<CannotVote>(new_state.own_vote)
        or boost::relaxed_get<Voted>(new_state.own_vote).empty();
    auto is_disputed = new_state.dispute_status.has_value();
    auto is_postponed = is_disputed
                          ? is_type<Postponed>(new_state.dispute_status.value())
                          : false;
    auto is_confirmed = is_disputed
                          ? is_type<Confirmed>(new_state.dispute_status.value())
                          : false;
    auto is_potential_spam = is_disputed  //
                         and not is_included and not is_backed
                         and not is_confirmed and not is_postponed;

    // We participate only in disputes which are not potential spam.
    auto allow_participation = not is_potential_spam;

    // This check is responsible for all clearing of spam slots. It runs
    // whenever a vote is imported from on or off chain, and decrements
    // slots whenever a candidate is newly backed, confirmed, or has our
    // own vote.
    if (not is_potential_spam) {
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
        SL_WARN(log_, "Rejecting import because of full spam slots");
        return outcome::success(false);
      }
    }

    // Participate in dispute if we did not cast a vote before and actually
    // have keys to cast a local vote. Disputes should fall in one of the
    // categories below, otherwise we will refrain from participation:
    // - `is_included` lands in prioritized queue
    // - `is_confirmed` | `is_backed` lands in the best effort queue
    // We don't participate in disputes escalated by disabled validators only.
    // We don't participate in disputes on finalized candidates.
    // see: {polkadot}/node/core/dispute-coordinator/src/initialized.rs:907

    if (own_vote_missing                      //
        and is_disputed and not is_postponed  //
        and allow_participation) {
      auto priority = static_cast<ParticipationPriority>(is_included);

      auto &receipt = new_state.votes.candidate_receipt;

      ParticipationRequest request{.candidate_hash = receipt.hash(*hasher_),
                                   .candidate_receipt = receipt,
                                   .session = session};

      auto res = participation_->queue_participation(priority, request);
      if (res.has_error()) {
        SL_ERROR(log_, "participation error: {}", res.error());
      }

    } else {
      SL_DEBUG(log_, "Will not queue participation for candidate");
    }

    // Also send any already existing approval vote on new disputes:
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L947
    is_freshly_disputed = not import_result.old_state.dispute_status.has_value()
                      and import_result.new_state.dispute_status.has_value();
    if (is_freshly_disputed and not is_postponed) {
      if (is_type<Voted>(new_state.own_vote)) {
        auto &own_votes = boost::relaxed_get<Voted>(new_state.own_vote);

        for (auto &[validator_index, dispute_statement, sig] : own_votes) {
          auto valid_dispute_statement_opt =
              if_type<ValidDisputeStatement>(dispute_statement);
          if (not valid_dispute_statement_opt.has_value()) {
            continue;
          }
          auto &valid_dispute_statement =
              valid_dispute_statement_opt.value().get();
          if (is_type<ApprovalChecking>(valid_dispute_statement)) {
            if (validator_index >= env.session.validators.size()) {
              SL_DEBUG(log_,
                       "Could not find pub key in `SessionInfo` for our own "
                       "approval vote!");
              continue;
            }
            auto &pub_key = env.session.validators[validator_index];

            SignedDisputeStatement statement{
                DisputeStatement{ValidDisputeStatement{ApprovalChecking{}}},
                candidate_hash,
                pub_key,
                sig,
                session};

            SL_TRACE(
                log_,
                "Sending out own approval vote. session={}, candidate_hash={}",
                session,
                candidate_hash);

            auto dispute_message_res = make_dispute_message(
                env.session, new_state.votes, statement, validator_index);

            if (dispute_message_res.has_value()) {
              auto &dispute_message = dispute_message_res.value();

              sendDisputeRequest(dispute_message, {});
            } else {
              SL_ERROR(
                  log_,
                  "No ongoing dispute, but we checked there is one! Error: {}",
                  dispute_message_res.error());
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
        OUTCOME_TRY(recent_disputes_opt, storage_->load_recent_disputes());
        auto recent_disputes = recent_disputes_opt.value_or(RecentDisputes{});

        auto [it, fresh] = recent_disputes.emplace(
            std::make_tuple(session, candidate_hash), Active{});

        if (fresh) {
          SL_INFO(log_,
                  "New dispute initiated for candidate "
                  "(session={}, candidate={})",
                  session,
                  candidate_hash);
        }

        // update status
        it->second = new_status;

        SL_TRACE(log_,
                 "Writing recent disputes with updates for candidate "
                 "(session={}, candidate={})",
                 session,
                 candidate_hash);
        storage_->write_recent_disputes(std::move(recent_disputes));
      }
    }

    // Notify ChainSelection if a dispute has concluded against a candidate.
    // ChainSelection will need to mark the candidate's relay parent as
    // reverted.
    if (is_freshly_concluded_against) {
      auto blocks_including =
          scraper_->get_blocks_including_candidate(candidate_hash);
      SL_TRACE(log_,
               "{} blocks include candidate={} concluded against",
               blocks_including.size(),
               candidate_hash);
      if (blocks_including.size() > 0) {
        std::vector<primitives::BlockHash> to_revert;
        to_revert.reserve(blocks_including.size());
        std::transform(blocks_including.begin(),
                       blocks_including.end(),
                       std::back_inserter(to_revert),
                       [](auto &block_info) { return block_info.hash; });
        std::ignore = block_tree_->markAsRevertedBlocks(std::move(to_revert));
        SL_DEBUG(
            log_, "Would be reverted up to {} blocks", blocks_including.size());
      } else {
        SL_DEBUG(log_,
                 "Could not find an including block for candidate against "
                 "which a dispute has concluded");
      }
    }

    SL_TRACE(log_,
             "Import summary:"
             "  candidate_hash={},"
             "  session={},"
             "  imported_approval_votes={},"
             "  imported_valid_votes={},"
             "  imported_invalid_votes={},"
             "  total_valid_votes={},"
             "  total_invalid_votes={},"
             "  confirmed={}",
             candidate_hash,
             session,
             import_result.imported_approval_votes,
             import_result.imported_valid_votes,
             import_result.imported_invalid_votes,
             import_result.new_state.votes.valid.size(),
             import_result.new_state.votes.invalid.size(),
             new_state.dispute_status.has_value()
                 ? is_type<Confirmed>(new_state.dispute_status.value())
                 : false);

    // Only write when votes have changed.
    if (import_result.imported_valid_votes
        || import_result.imported_invalid_votes) {
      storage_->write_candidate_votes(
          session, candidate_hash, import_result.new_state.votes);
    }

    // Update metrics
    if (is_freshly_disputed) {
      metric_disputes_total_->inc();
    }

    metric_votes_valid_->inc(import_result.imported_valid_votes);
    metric_votes_invalid_->inc(import_result.imported_invalid_votes);

    if (is_freshly_concluded_for) {
      metric_concluded_valid_->inc();
    }
    if (is_freshly_concluded_against) {
      metric_concluded_invalid_->inc();
    }

    return outcome::success(true);
  }

  outcome::result<network::DisputeMessage>
  DisputeCoordinatorImpl::make_dispute_message(SessionInfo session_info,
                                               CandidateVotes votes,
                                               SignedDisputeStatement our_vote,
                                               ValidatorIndex our_index) {
    auto &validators = session_info.validators;

    ValidatorIndex other_index;

    auto get_other_vote =
        [&](auto &some_kind_votes) -> outcome::result<SignedDisputeStatement> {
      if (some_kind_votes.empty()) {
        return DisputeMessageCreationError::NoOppositeVote;
      }

      auto it = some_kind_votes.begin();

      auto validator_index = it->first;
      auto &[statement_kind, validator_signature] = it->second;

      if (validator_index >= validators.size()) {
        return DisputeMessageCreationError::InvalidValidatorIndex;
      }
      auto validator_public = validators[validator_index];

      // check sig

      DisputeStatement statement{statement_kind};

      OUTCOME_TRY(
          payload,
          getSignablePayload(
              statement, our_vote.candidate_hash, our_vote.session_index));

      OUTCOME_TRY(is_valid,
                  sr25519_crypto_provider_->verify(
                      validator_signature, payload, validator_public));

      if (not is_valid) {
        return DisputeMessageCreationError::InvalidStoredStatement;
      }

      // make another signed statement

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

    if (candidate_receipt.hash(*hasher_) != candidate_hash) {
      return DisputeMessageConstructingError::InvalidCandidateReceipt;
    }

    auto valid_kind_opt =
        if_type<ValidDisputeStatement>(valid_statement.dispute_statement);
    if (not valid_kind_opt.has_value()) {
      return DisputeMessageConstructingError::ValidStatementHasInvalidKind;
    }
    auto &valid_kind = valid_kind_opt->get();

    auto invalid_kind_opt =
        if_type<InvalidDisputeStatement>(invalid_statement.dispute_statement);
    if (not invalid_kind_opt.has_value()) {
      return DisputeMessageConstructingError::InvalidStatementHasValidKind;
    }
    auto &invalid_kind = invalid_kind_opt->get();

    network::ValidDisputeVote valid_vote{
        .index = valid_index,
        .signature = valid_statement.validator_signature,
        .kind = valid_kind,
    };

    network::InvalidDisputeVote invalid_vote{
        .index = invalid_index,
        .signature = invalid_statement.validator_signature,
        .kind = invalid_kind,
    };

    return network::DisputeMessage{
        candidate_receipt,
        session_index,
        invalid_vote,
        valid_vote,
    };
  }

  void DisputeCoordinatorImpl::send_dispute_messages(
      const CandidateEnvironment &env, const CandidateVoteState &vote_state) {
    auto votes = if_type<const Voted>(vote_state.own_vote);
    if (not votes.has_value()) {
      return;
    }

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/lib.rs#L450
    for (auto &[validator_index, dispute_statement, validator_signature] :
         votes.value().get()) {
      if (validator_index >= env.session.validators.size()) {
        SL_DEBUG(log_,
                 "Could not find our own key in `SessionInfo` "
                 "(session={}, validator_index={})",
                 env.session_index,
                 validator_index);
        continue;
      }
      auto &validator_public = env.session.validators[validator_index];

      auto &candidate_hash = vote_state.votes.candidate_receipt.hash(*hasher_);

      auto payload_res = getSignablePayload(
          dispute_statement, candidate_hash, env.session_index);
      if (payload_res.has_error()) {
        SL_WARN(log_,
                "Sending dispute vote produces bad payload! {}",
                payload_res.error());
        continue;
      }
      auto &payload = payload_res.value();

      auto validation_res = sr25519_crypto_provider_->verify(
          validator_signature, payload, validator_public);

      if (validation_res.has_error()) {
        SL_ERROR(log_,
                 "Checking our own signature failed: {}; db corruption?",
                 validation_res.error());
        continue;
      }
      if (not validation_res.value()) {
        SL_ERROR(log_,
                 "Checking our own signature failed: invalid; db corruption?",
                 validation_res.error());
        continue;
      }

      SignedDisputeStatement our_vote_signed{
          dispute_statement,
          candidate_hash,
          validator_public,
          validator_signature,
          env.session_index,
      };

      auto msg_res = make_dispute_message(
          env.session, vote_state.votes, our_vote_signed, validator_index);
      if (msg_res.has_error()) {
        SL_DEBUG(log_, "Creating dispute message failed: {}", msg_res.error());
        continue;
      }
      auto &dispute_message = msg_res.value();

      sendDisputeRequest(dispute_message, {});
    }
  }

  outcome::result<void> DisputeCoordinatorImpl::issue_local_statement(
      const CandidateHash &candidate_hash,
      const CandidateReceipt &candidate_receipt,
      SessionIndex session,
      bool valid) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L1102

    SL_TRACE(log_,
             "Issuing local statement for candidate! "
             "session={}, candidate_hash={}, relay_parent={}",
             session,
             candidate_hash,
             candidate_receipt.descriptor.relay_parent);

    // Load environment:

    OUTCOME_TRY(
        session_info_opt,
        api_->session_info(candidate_receipt.descriptor.relay_parent, session));
    if (not session_info_opt.has_value()) {
      SL_WARN(log_, "Missing info for session which has an active dispute");
      return outcome::success();
    }

    CandidateEnvironment env{
        .session_index = session,
        .session = std::move(session_info_opt.value()),
    };

    auto keypair = session_keys_->getParaKeyPair(env.session.validators);
    if (keypair.has_value()) {
      env.controlled_indices.emplace(keypair->second);
    }

    CandidateVotes votes;

    OUTCOME_TRY(old_state_opt,
                storage_->load_candidate_votes(session, candidate_hash));
    if (old_state_opt.has_value()) {
      votes = {
          .candidate_receipt = old_state_opt->candidate_receipt,
          .valid = old_state_opt->valid,
          .invalid = old_state_opt->invalid,
      };
    } else {
      votes = {
          .candidate_receipt = candidate_receipt,
      };
    }

    // Sign a statement for each validator index we control which has
    // not already voted. This should generally be maximum 1 statement.
    std::set<ValidatorIndex> voted_indices;
    for (auto &[key, _] : votes.valid) {
      voted_indices.emplace(key);
    }
    for (auto &[key, _] : votes.invalid) {
      voted_indices.emplace(key);
    }

    std::vector<Indexed<SignedDisputeStatement>> statements;

    const auto &controlled_indices = env.controlled_indices;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L1143
    for (auto index : controlled_indices) {
      if (voted_indices.find(index) != voted_indices.end()) {
        continue;
      }

      auto dispute_statement =
          valid ? DisputeStatement{ValidDisputeStatement{Explicit{}}}
                : DisputeStatement{InvalidDisputeStatement{Explicit{}}};

      auto payload_res =
          getSignablePayload(dispute_statement, candidate_hash, session);
      BOOST_ASSERT(payload_res.has_value());
      auto &payload = payload_res.value();

      OUTCOME_TRY(signature,
                  sr25519_crypto_provider_->sign(*keypair->first, payload));

      Indexed<SignedDisputeStatement> statement{{
                                                    dispute_statement,
                                                    candidate_hash,
                                                    keypair->first->public_key,
                                                    signature,
                                                    session,
                                                },
                                                index};

      statements.emplace_back(std::move(statement));
    }

    // Get our message out:
    for (auto &[statement, index] : statements) {
      auto dispute_message_res =
          make_dispute_message(env.session, votes, statement, index);
      if (dispute_message_res.has_error()) {
        SL_ERROR(log_,
                 "Creating dispute message failed: {}",
                 dispute_message_res.error());
        continue;
      }
      auto &dispute_message = dispute_message_res.value();

      sendDisputeRequest(dispute_message, {});
    }

    // Do import
    if (not statements.empty()) {
      OUTCOME_TRY(
          is_ok,
          handle_import_statements(candidate_receipt, session, statements));

      if (is_ok) {
        SL_TRACE(log_,
                 "`handle_import_statements` successfully imported our vote!");
      } else {
        SL_ERROR(log_,
                 "`handle_import_statements` considers our own votes invalid!");
      }
    }

    return outcome::success();
  }

  void DisputeCoordinatorImpl::importStatements(
      CandidateReceipt candidate_receipt,
      SessionIndex session,
      std::vector<Indexed<SignedDisputeStatement>> statements,
      CbOutcome<void> &&cb) {
    REINVOKE(*dispute_thread_handler_,
             importStatements,
             std::move(candidate_receipt),
             session,
             std::move(statements),
             std::move(cb));

    SL_TRACE(log_, "DisputeCoordinatorMessage::ImportStatements");

    auto res = handle_import_statements(candidate_receipt, session, statements);
    if (res.has_error()) {
      return cb(res.as_failure());
    }

    [[maybe_unused]] auto &valid_import = res.value();

    return cb(outcome::success());
  }

  void DisputeCoordinatorImpl::getRecentDisputes(
      CbOutcome<OutputDisputes> &&cb) {
    REINVOKE(*dispute_thread_handler_, getRecentDisputes, std::move(cb));

    SL_TRACE(log_, "Loading recent disputes from db");

    auto recent_disputes_res = storage_->load_recent_disputes();
    if (recent_disputes_res.has_error()) {
      return cb(recent_disputes_res.as_failure());
    }
    auto recent_disputes =
        recent_disputes_res.value().value_or(RecentDisputes{});

    SL_TRACE(log_, "Loaded recent disputes from db");

    OutputDisputes output;
    output.reserve(recent_disputes.size());
    std::transform(
        recent_disputes.begin(),
        recent_disputes.end(),
        std::back_inserter(output),
        [](const auto &p)
            -> std::tuple<SessionIndex, CandidateHash, DisputeStatus> {
          return {std::get<0>(p.first), std::get<1>(p.first), p.second};
        });

    cb(std::move(output));
  }

  void DisputeCoordinatorImpl::getActiveDisputes(
      CbOutcome<OutputDisputes> &&cb) {
    REINVOKE(*dispute_thread_handler_, getActiveDisputes, std::move(cb));

    SL_TRACE(log_, "DisputeCoordinatorMessage::ActiveDisputes");

    auto recent_disputes_res = storage_->load_recent_disputes();
    if (recent_disputes_res.has_error()) {
      return cb(recent_disputes_res.as_failure());
    }
    auto recent_disputes =
        recent_disputes_res.value().value_or(RecentDisputes{});

    OutputDisputes output;

    auto now = system_clock_.nowUint64();
    for (const auto &p : recent_disputes) {
      const auto &status = p.second;

      auto at = visit_in_place(
          status,
          [](const Active &) -> std::optional<Timestamp> {
            return std::nullopt;
          },
          [](const Confirmed &) -> std::optional<Timestamp> {
            return std::nullopt;
          },
          [](const ConcludedFor &at) -> std::optional<Timestamp> {
            return (Timestamp)at;
          },
          [](const ConcludedAgainst &at) -> std::optional<Timestamp> {
            return (Timestamp)at;
          },
          [](const Postponed &) -> std::optional<Timestamp> {
            return std::nullopt;
          });

      auto dispute_is_inactive =
          at.has_value() and at.value() + kActiveDurationSecs < now;

      if (not dispute_is_inactive) {
        output.emplace_back(
            std::get<0>(p.first), std::get<1>(p.first), p.second);
      }
    }

    cb(std::move(output));
  }

  void DisputeCoordinatorImpl::queryCandidateVotes(
      const QueryCandidateVotes &query, CbOutcome<OutputCandidateVotes> &&cb) {
    REINVOKE(
        *dispute_thread_handler_, queryCandidateVotes, query, std::move(cb));

    SL_TRACE(log_, "DisputeCoordinatorMessage::QueryCandidateVotes");

    std::vector<std::tuple<SessionIndex, CandidateHash, CandidateVotes>> output;

    for (auto &[session, candidate_hash] : query) {
      auto state_res = storage_->load_candidate_votes(session, candidate_hash);
      if (state_res.has_error()) {
        cb(state_res.as_failure());
        return;
      }
      auto &state_opt = state_res.value();
      if (state_opt.has_value()) {
        output.push_back(std::make_tuple(
            session, candidate_hash, std::move(state_opt.value())));
      } else {
        SL_DEBUG(log_, "No votes found for candidate");
      }
    }

    cb(std::move(output));
  }

  void DisputeCoordinatorImpl::issueLocalStatement(
      SessionIndex session,
      CandidateHash candidate_hash,
      CandidateReceipt candidate_receipt,
      bool valid) {
    REINVOKE(*dispute_thread_handler_,
             issueLocalStatement,
             session,
             candidate_hash,
             std::move(candidate_receipt),
             valid);

    SL_TRACE(log_,
             "DisputeCoordinatorMessage::IssueLocalStatement. "
             "session={}, candidate_hash={}, relay_parent={}",
             session,
             candidate_hash,
             candidate_receipt.descriptor.relay_parent);
    auto res = issue_local_statement(
        candidate_hash, candidate_receipt, session, valid);

    if (res.has_error()) {
      SL_ERROR(log_, "Error during issue local statement: {}", res.error());
    }
  }

  void DisputeCoordinatorImpl::determineUndisputedChain(
      primitives::BlockInfo base,
      std::vector<BlockDescription> block_descriptions,
      CbOutcome<primitives::BlockInfo> &&cb) {
    REINVOKE(*dispute_thread_handler_,
             determineUndisputedChain,
             base,
             std::move(block_descriptions),
             std::move(cb));

    SL_TRACE(log_, "DisputeCoordinatorMessage::DetermineUndisputedChain");

    auto res =
        determine_undisputed_chain(base.number, base.hash, block_descriptions);

    if (res.has_error()) {
      return cb(res.as_failure());
    }
    auto &undisputed_chain = res.value();

    // Update finality lag if possible
    if (not block_descriptions.empty()) {
      if (auto number_res = block_header_repository_->getNumberByHash(
              block_descriptions.back().block_hash);
          number_res.has_value()) {
        if (number_res.value() > undisputed_chain.number) {
          metric_disputes_finality_lag_->set(number_res.value()
                                             - undisputed_chain.number);
        } else {
          metric_disputes_finality_lag_->set(0);
        }
      }
    } else {
      metric_disputes_finality_lag_->set(0);
    }

    cb(std::move(undisputed_chain));
  }

  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/initialized.rs#L1272
  outcome::result<primitives::BlockInfo>
  DisputeCoordinatorImpl::determine_undisputed_chain(
      const primitives::BlockNumber &base_number,
      const primitives::BlockHash &base_hash,
      std::vector<BlockDescription> descriptions) {
    primitives::BlockInfo last(base_number, base_hash);

    if (not descriptions.empty()) {
      last = {last.number + primitives::BlockNumber(descriptions.size()),
              descriptions.back().block_hash};
    }

    // Fast path for no disputes.
    OUTCOME_TRY(recent_disputes_opt, storage_->load_recent_disputes());

    if (not recent_disputes_opt.has_value()) {
      return last;
    }
    const auto &recent_disputes = recent_disputes_opt.value();

    if (recent_disputes.empty()) {
      return last;
    }

    /// Whether the disputed candidate is possibly invalid.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/primitives/src/disputes/status.rs#L110-L111
    auto is_possibly_invalid = [&recent_disputes](
                                   SessionIndex session,
                                   const CandidateHash &candidate_hash) {
      auto it = recent_disputes.find(std::tie(session, candidate_hash));
      if (it == recent_disputes.end()) {
        return false;
      }
      return visit_in_place(
          it->second,  // status
          [](const ConcludedFor &) { return false; },
          [](const auto &) { return true; });
    };

    last = {base_number, base_hash};

    for (const auto &description : descriptions) {
      auto has_disputed_candidate = std::any_of(
          description.candidates.begin(),
          description.candidates.end(),
          [&](const auto &candidate_hash) {
            return is_possibly_invalid(description.session, candidate_hash);
          });
      if (has_disputed_candidate) {
        return last;
      }

      last = {last.number + 1, description.block_hash};
    }

    return last;
  }

  void DisputeCoordinatorImpl::onDisputeRequest(
      const libp2p::peer::PeerId &peer_id,
      const network::DisputeMessage &request,
      CbOutcome<void> &&cb) {
    REINVOKE(*dispute_thread_handler_,
             onDisputeRequest,
             peer_id,
             request,
             std::move(cb));

    // Only accept messages from validators, in case there are multiple
    // `AuthorityId`s, we just take the first one. On session boundaries
    // this might allow validators to double their rate limit for a short
    // period of time, which seems acceptable.
    auto authority_id_opt = authority_discovery_->get(peer_id);
    if (not authority_id_opt.has_value()) {
      SL_DEBUG(log_, "Peer {} is not validator - dropping message", peer_id);
      // TODO reputation_changes: vec![COST_NOT_A_VALIDATOR],
      return sendDisputeResponse(DisputeProcessingError::NotAValidator,
                                 std::move(cb));
    }
    auto &authority_id = authority_id_opt.value();

    /// Push an incoming request for a given authority.

    auto &queue = queues_[authority_id];

    if (queue.size() >= kPeerQueueCapacity) {
      SL_DEBUG(log_, "Peer {} hit the rate limit - dropping message", peer_id);
      // TODO reputation_changes: vec![COST_APPARENT_FLOOD],
      return sendDisputeResponse(DisputeProcessingError::AuthorityFlooding,
                                 std::move(cb));
    }
    queue.emplace_back(request,
                       [wp{weak_from_this()},
                        cb{std::move(cb)}](outcome::result<void> res) mutable {
                         if (auto self = wp.lock()) {
                           self->sendDisputeResponse(res, std::move(cb));
                         }
                       });

    // We have at least one element to process - rate limit `timer` needs to
    // exist now:
    make_task_for_next_portion();

    return;
  }

  void DisputeCoordinatorImpl::sendDisputeResponse(outcome::result<void> res,
                                                   CbOutcome<void> &&cb) {
    REINVOKE(*main_pool_handler_,
             sendDisputeResponse,
             std::move(res),
             std::move(cb));
    cb(res);
  }

  void DisputeCoordinatorImpl::make_task_for_next_portion() {
    if (not rate_limit_timer_.has_value()) {
      rate_limit_timer_ =
          scheduler_->scheduleWithHandle([wp{weak_from_this()}]() {
            if (auto self = wp.lock()) {
              BOOST_ASSERT(self->dispute_thread_handler_->isInCurrentThread());
              self->process_portion_incoming_disputes();
            }
          });
    }
  }

  void DisputeCoordinatorImpl::process_portion_incoming_disputes() {
    if (rate_limit_timer_) {
      rate_limit_timer_.reset();
    }

    std::vector<std::tuple<libp2p::peer::PeerId,
                           network::DisputeMessage,
                           CbOutcome<void>>>
        heads;
    heads.reserve(queues_.size());

    auto old_queues = std::move(queues_);

    for (auto &[auth, queue] : old_queues) {
      auto peer_opt = authority_discovery_->get(auth);
      if (not peer_opt.has_value()) {
        continue;
      }
      const auto &peer_id = peer_opt->id;

      BOOST_ASSERT_MSG(not queue.empty(),
                       "Invariant that queues are never empty is broken.");

      auto &[request, cb] = queue.front();
      heads.emplace_back(peer_id, std::move(request), std::move(cb));
      queue.pop_front();

      if (not queue.empty()) {
        queues_.emplace(auth, queue);
      }
    }

    if (not queues_.empty()) {
      // Still not empty - we should get woken at some point.
      make_task_for_next_portion();
    }

    for (auto &[peer, request, cb] : std::move(heads)) {
      // No early return - we cannot cancel imports of one peer, because the
      // import of another failed:
      auto res = start_import_or_batch(peer, std::move(request), std::move(cb));
      if (res.has_error()) {
        SL_ERROR(log_, "Can't start import or batch: {}", res.error());
      }
    }
  }

  outcome::result<void> DisputeCoordinatorImpl::start_import_or_batch(
      const libp2p::peer::PeerId &peer,
      const network::DisputeMessage &request,
      CbOutcome<void> &&cb) {
    OUTCOME_TRY(info,
                runtime_info_->get_session_info_by_index(
                    request.candidate_receipt.descriptor.relay_parent,
                    request.session_index));

    const auto &[candidate_receipt,
                 session_index,
                 unchecked_valid_vote,
                 unchecked_invalid_vote] = request;
    const auto &candidate_hash = candidate_receipt.hash(*hasher_);

    const auto &session_info = info.session_info;

    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/primitives/src/disputes/message.rs#L232

    // vote valid
    Indexed<SignedDisputeStatement> checked_valid_vote;
    {
      const auto &[validator_index, signature, kind] = unchecked_valid_vote;
      if (validator_index >= session_info.validators.size()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return DisputeMessageCreationError::InvalidValidatorIndex;
      }
      auto validator_public = session_info.validators[validator_index];

      DisputeStatement dispute_statement{kind};

      auto payload_res =
          getSignablePayload(dispute_statement, candidate_hash, session_index);
      if (payload_res.has_error()) {
        return payload_res.as_failure();
      }
      auto &payload = payload_res.value();

      auto validation_res = sr25519_crypto_provider_->verify(
          signature, payload, validator_public);

      if (validation_res.has_error()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return SignatureValidationError::InvalidSignature;
      }
      if (not validation_res.value()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return SignatureValidationError::InvalidSignature;
      }

      checked_valid_vote = {{
                                dispute_statement,
                                candidate_hash,
                                validator_public,
                                signature,
                                session_index,
                            },
                            validator_index};
    }

    Indexed<SignedDisputeStatement> checked_invalid_vote;
    {
      const auto &[validator_index, signature, kind] = unchecked_invalid_vote;
      if (validator_index >= session_info.validators.size()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return DisputeMessageCreationError::InvalidValidatorIndex;
      }
      auto validator_public = session_info.validators[validator_index];

      DisputeStatement dispute_statement{kind};

      auto payload_res =
          getSignablePayload(dispute_statement, candidate_hash, session_index);
      if (payload_res.has_error()) {
        return payload_res.as_failure();
      }
      auto &payload = payload_res.value();

      auto validation_res = sr25519_crypto_provider_->verify(
          signature, payload, validator_public);

      if (validation_res.has_error()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return SignatureValidationError::InvalidSignature;
      }
      if (not validation_res.value()) {
        // TODO reputation_changes: vec![COST_INVALID_SIGNATURE],
        return SignatureValidationError::InvalidSignature;
      }

      checked_invalid_vote = {{
                                  dispute_statement,
                                  candidate_hash,
                                  validator_public,
                                  signature,
                                  session_index,
                              },
                              validator_index};
    }

    auto &valid_vote = checked_valid_vote;
    auto &invalid_vote = checked_invalid_vote;

    // Find or create batch
    OUTCOME_TRY(found_batch,
                batches_->find_batch(candidate_hash, candidate_receipt));
    auto [batch, just_created] = std::move(found_batch);

    if (just_created) {
      // There was no entry yet - start import immediately:

      SL_TRACE(
          log_,
          "No batch yet - triggering immediate import (candidate={}, peer={})",
          candidate_hash,
          peer);

      PreparedImport prepared_import{
          .candidate_receipt = batch->candidate_receipt,
          .statements = {valid_vote, invalid_vote},
          .requesters = {{peer, std::move(cb)}},
      };

      start_import(std::move(prepared_import));

      return outcome::success();
    }

    SL_TRACE(
        log_, "Batch exists - batching request (candidate={})", candidate_hash);

    auto cb_opt =
        batch->add_votes(valid_vote, invalid_vote, peer, std::move(cb));

    // Returned value means duplicate
    if (cb_opt.has_value()) {
      // We don't expect honest peers to send redundant votes  within a  single
      // batch, as the timeout for retry is much higher. Still we don't want to
      // punish the node as it might not be the node's fault. Some other
      // (malicious) node could have been faster sending the same votes in order
      // to harm the reputation of that honest node. Given that we already have
      // a rate limit, if a validator chooses to waste available rate with
      // redundant votes - so be it. The actual dispute resolution is
      // unaffected.

      SL_DEBUG(log_,
               "Peer {} sent completely redundant votes within a single batch "
               "- that looks fishy!",
               peer);

      // While we have seen duplicate votes, we cannot confirm as we don't know
      // yet whether the batch is going to be confirmed, so we assume the worst.
      // We don't want to push the pending response to the batch either as that
      // would be unbounded, only limited by the rate limit.

      (*cb_opt)(BatchError::RedundantMessage);
    }

    check_batches();

    return outcome::success();
  }

  void DisputeCoordinatorImpl::check_batches() {
    if (not batch_collecting_timer_.has_value()) {
      batch_collecting_timer_ = scheduler_->scheduleWithHandle(
          [wp{weak_from_this()}]() {
            if (auto self = wp.lock()) {
              BOOST_ASSERT(self->dispute_thread_handler_->isInCurrentThread());
              self->batch_collecting_timer_.reset();
              auto ready_prepared_imports = self->batches_->check_batches();
              for (auto &prepared_import : ready_prepared_imports) {
                self->start_import(std::move(prepared_import));
              }
              if (not self->batches_->empty()) {
                self->check_batches();
              }
            }
          },
          Batch::kBatchCollectingInterval);
    }
  }

  // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/network/dispute-distribution/src/receiver/mod.rs#L422
  void DisputeCoordinatorImpl::start_import(PreparedImport &&prepared_import) {
    auto [candidate_receipt, statements, requesters] =
        std::move(prepared_import);

    if (statements.empty()) {
      SL_DEBUG(log_,
               "Not importing empty batch (candidate={})",
               candidate_receipt.hash(*hasher_));
      return;
    }

    auto &[signed_statement, validator_index] = statements.front();
    auto &session_index = signed_statement.session_index;
    // auto &candidate_hash = signed_statement.candidate_hash;

    auto pending_confirmation =
        [wp{weak_from_this()},
         requesters(std::move(requesters))](outcome::result<void> res) mutable {
          if (auto self = wp.lock()) {
            for (auto &[peer, cb] : requesters) {
              self->sendDisputeResponse(res, std::move(cb));
            }
          }
        };

    importStatements(candidate_receipt,
                     session_index,
                     statements,
                     std::move(pending_confirmation));
  }

  void DisputeCoordinatorImpl::sendDisputeRequest(
      const network::DisputeMessage &request, CbOutcome<void> &&cb) {
    auto &candidate_hash = request.candidate_receipt.hash(*hasher_);

    for (auto &[_candidate_hash, _] : sending_disputes_) {
      if (_candidate_hash == candidate_hash) {
        SL_TRACE(log_,
                 "Dispute (candidate={}) sending already active.",
                 candidate_hash);
        return;
      }
    }

    auto protocol = router_->getSendDisputeProtocol();
    BOOST_ASSERT_MSG(protocol,
                     "Router did not provide `send dispute` protocol");

    auto &[_, sending_dispute] = sending_disputes_.emplace_back(
        candidate_hash,
        std::make_unique<SendingDispute>(
            log_, main_pool_handler_, authority_discovery_, protocol, request));

    std::ignore =
        sending_dispute->refresh_sends(*runtime_info_, active_sessions_);
  }

  void DisputeCoordinatorImpl::getDisputeForInherentData(
      const primitives::BlockInfo &relay_parent,
      std::function<void(MultiDisputeStatementSet)> &&cb) {
    SL_TRACE(log_, "Selecting disputes; relay_parent {}", relay_parent);

    if (has_required_runtime(relay_parent)) {
      SL_TRACE(log_,
               "Selected disputes for {} (prioritized selection)",
               relay_parent);

      PrioritizedSelection selection(
          system_clock_, api_, shared_from_this(), log_);

      auto disputes = selection.select_disputes(relay_parent);

      cb(std::move(disputes));
      return;
    }

    SL_TRACE(log_, "Selected disputes for {} (random selection)", relay_parent);

    RandomSelection selection(shared_from_this(), log_);

    auto disputes = selection.select_disputes();
    cb(std::move(disputes));
  }

  bool DisputeCoordinatorImpl::has_required_runtime(
      const primitives::BlockInfo &relay_parent) {
    SL_TRACE(log_,
             "Fetching ParachainHost runtime api version for relay_parent {}",
             relay_parent);

    auto version_res = core_api_->version(relay_parent.hash);

    if (version_res.has_error()) {
      SL_TRACE(log_,
               "Execution error while fetching ParachainHost runtime api "
               "version for relay_parent {}: {}",
               relay_parent,
               version_res.error());
      return false;
    }
    auto &version = version_res.value();

    auto &apis = version.apis;

    // usage in lambda is not detected for some reason causing a warning
    [[maybe_unused]] static const common::Hash64 parachain_host_api_hash =
        hasher_->blake2b_64(common::Buffer::fromString("ParachainHost"));

    auto it = std::find_if(apis.begin(), apis.end(), [](auto &api_version) {
      return api_version.first == parachain_host_api_hash;
    });
    if (it == apis.end()) {
      SL_TRACE(log_,
               "Execution error while fetching ParachainHost runtime api "
               "version for relay_parent={}: such api not found",
               relay_parent);
      return false;
    }

    auto parachain_host_api_version = it->second;

    if (parachain_host_api_version
        >= kPrioritizedSelectionRuntimeVersionRequirement) {
      SL_TRACE(log_,
               "Fetched ParachainHost runtime api version for relay_parent {} "
               "is {}; it's suitable version",
               relay_parent,
               parachain_host_api_version);
      return true;
    }

    SL_TRACE(
        log_,
        "Fetched ParachainHost runtime api version for relay_parent {} is {}; "
        "it isn't suitable version",
        relay_parent,
        parachain_host_api_version);
    return false;
  }

}  // namespace kagome::dispute

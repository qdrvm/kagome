/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_processor.hpp"

#include <array>
#include <gsl/span>
#include <unordered_map>

#include <erasure_coding/erasure_coding.h>

#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/impl/runtime_info.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/candidate_view.hpp"
#include "parachain/peer_relay_parent_knowledge.hpp"
#include "scale/scale.hpp"
#include "utils/async_sequence.hpp"
#include "utils/map.hpp"
#include "utils/profiler.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain,
                            ParachainProcessorImpl::Error,
                            e) {
  using E = kagome::parachain::ParachainProcessorImpl::Error;
  switch (e) {
    case E::RESPONSE_ALREADY_RECEIVED:
      return "Response already present";
    case E::COLLATION_NOT_FOUND:
      return "Collation not found";
    case E::UNDECLARED_COLLATOR:
      return "Undeclared collator";
    case E::KEY_NOT_PRESENT:
      return "Private key is not present";
    case E::VALIDATION_FAILED:
      return "Validate and make available failed";
    case E::VALIDATION_SKIPPED:
      return "Validate and make available skipped";
    case E::OUT_OF_VIEW:
      return "Out of view";
    case E::DUPLICATE:
      return "Duplicate";
    case E::NO_INSTANCE:
      return "No self instance";
    case E::NOT_A_VALIDATOR:
      return "Node is not a validator";
    case E::NOT_SYNCHRONIZED:
      return "Node not synchronized";
    case E::PEER_LIMIT_REACHED:
      return "Peer limit reached";
    case E::PROTOCOL_MISMATCH:
      return "Protocol mismatch";
  }
  return "Unknown parachain processor error";
}

namespace {
  constexpr const char *kIsParachainValidator =
      "kagome_node_is_parachain_validator";
}

namespace kagome::parachain {
  constexpr size_t kMinGossipPeers = 25;

  ParachainProcessorImpl::ParachainProcessorImpl(
      std::shared_ptr<network::PeerManager> pm,
      std::shared_ptr<dispute::RuntimeInfo> runtime_info,
      std::shared_ptr<crypto::Sr25519Provider> crypto_provider,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<boost::asio::io_context> this_context,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<ThreadPool> thread_pool,
      std::shared_ptr<parachain::BitfieldSigner> bitfield_signer,
      std::shared_ptr<parachain::PvfPrecheck> pvf_precheck,
      std::shared_ptr<parachain::BitfieldStore> bitfield_store,
      std::shared_ptr<parachain::BackingStore> backing_store,
      std::shared_ptr<parachain::Pvf> pvf,
      std::shared_ptr<parachain::AvailabilityStore> av_store,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory,
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable,
      std::shared_ptr<authority_discovery::Query> query_audi,
      std::shared_ptr<ProspectiveParachains> prospective_parachains)
      : pm_(std::move(pm)),
        runtime_info_(std::move(runtime_info)),
        crypto_provider_(std::move(crypto_provider)),
        router_(std::move(router)),
        this_context_(std::make_shared<ThreadHandler>(std::move(this_context))),
        hasher_(std::move(hasher)),
        peer_view_(std::move(peer_view)),
        pvf_(std::move(pvf)),
        signer_factory_(std::move(signer_factory)),
        bitfield_signer_(std::move(bitfield_signer)),
        pvf_precheck_(std::move(pvf_precheck)),
        bitfield_store_(std::move(bitfield_store)),
        backing_store_(std::move(backing_store)),
        av_store_(std::move(av_store)),
        parachain_host_(std::move(parachain_host)),
        app_config_(app_config),
        babe_status_observable_(std::move(babe_status_observable)),
        query_audi_{std::move(query_audi)},
        in_pool_thread_handler_{thread_pool->handler()},
        prospective_parachains_{std::move(prospective_parachains)} {
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(this_context_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(bitfield_signer_);
    BOOST_ASSERT(bitfield_store_);
    BOOST_ASSERT(backing_store_);
    BOOST_ASSERT(pvf_);
    BOOST_ASSERT(av_store_);
    BOOST_ASSERT(parachain_host_);
    BOOST_ASSERT(signer_factory_);
    BOOST_ASSERT(babe_status_observable_);
    BOOST_ASSERT(query_audi_);
    BOOST_ASSERT(prospective_parachains_);
    app_state_manager->takeControl(*this);

    metrics_registry_->registerGaugeFamily(
        kIsParachainValidator,
        "Tracks if the validator participates in parachain consensus. "
        "Parachain validators are a subset of the active set validators that "
        "perform approval checking of all parachain candidates in a session. "
        "Updates at session boundary.");
    metric_is_parachain_validator_ =
        metrics_registry_->registerGaugeMetric(kIsParachainValidator);
    metric_is_parachain_validator_->set(false);
  }

  bool ParachainProcessorImpl::prepare() {
    bitfield_signer_->setBroadcastCallback(
        [log{logger_}, wptr_self{weak_from_this()}](
            const primitives::BlockHash &relay_parent,
            const network::SignedBitfield &bitfield) {
          log->info("Distribute bitfield on {}", relay_parent);
          if (auto self = wptr_self.lock()) {
            auto msg = std::make_shared<
                network::WireMessage<network::ValidatorProtocolMessage>>(
                network::BitfieldDistribution{relay_parent, bitfield});

            self->pm_->getStreamEngine()->broadcast(
                self->router_->getValidationProtocol(), msg);
          }
        });

    babe_status_observer_ =
        std::make_shared<primitives::events::BabeStateEventSubscriber>(
            babe_status_observable_, false);
    babe_status_observer_->subscribe(
        babe_status_observer_->generateSubscriptionSetId(),
        primitives::events::BabeStateEventType::kSyncState);
    babe_status_observer_->setCallback(
        [wself{weak_from_this()}, was_synchronized = false](
            auto /*set_id*/,
            bool &synchronized,
            auto /*event_type*/,
            const primitives::events::BabeStateEventParams &event) mutable {
          if (auto self = wself.lock()) {
            if (event == consensus::babe::Babe::State::SYNCHRONIZED) {
              if (not was_synchronized) {
                self->bitfield_signer_->start(
                    self->peer_view_->intoChainEventsEngine());
                self->pvf_precheck_->start(
                    self->peer_view_->intoChainEventsEngine());
                was_synchronized = true;
              }
            }
            if (was_synchronized) {
              if (!synchronized) {
                synchronized = true;
                auto my_view = self->peer_view_->getMyView();
                if (!my_view) {
                  SL_WARN(self->logger_,
                          "Broadcast my view failed, "
                          "because my view still not exists.");
                  return;
                }

                SL_TRACE(self->logger_,
                         "Broadcast my view because synchronized.");
                self->broadcastView(my_view->get().view);
              }
            }
          }
        });

    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        peer_view_->intoChainEventsEngine());
    chain_sub_->subscribe(
        chain_sub_->generateSubscriptionSetId(),
        primitives::events::ChainEventType::kDeactivateAfterFinalization);
    chain_sub_->setCallback(
        [wptr{weak_from_this()}](
            auto /*set_id*/,
            auto && /*internal_obj*/,
            auto /*event_type*/,
            const primitives::events::ChainEventParams &event) {
          if (auto self = wptr.lock()) {
            self->onDeactivateBlocks(event);
          }
        });

    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    my_view_sub_->subscribe(my_view_sub_->generateSubscriptionSetId(),
                            network::PeerView::EventType::kViewUpdated);
    my_view_sub_->setCallback(
        [wptr{weak_from_this()}](auto /*set_id*/,
                                 auto && /*internal_obj*/,
                                 auto /*event_type*/,
                                 const network::ExView &event) {
          if (auto self = wptr.lock()) {
            self->onViewUpdated(event);
          }
        });
    return true;
  }

  void ParachainProcessorImpl::onViewUpdated(const network::ExView &event) {
    REINVOKE(*this_context_, onViewUpdated, event);

    const auto &relay_parent =
        primitives::calculateBlockHash(event.new_head, *hasher_).value();

    for (const auto &lost : event.lost) {
      SL_TRACE(logger_, "Removed backing task.(relay parent={})", lost);

      our_current_state_.state_by_relay_parent.erase(lost);
      pending_candidates.erase(lost);
      our_current_state_.active_leaves.erase(lost);
    }
    our_current_state_.active_leaves[relay_parent] = {};

    if (auto r = canProcessParachains(); r.has_error()) {
      return;
    }

    createBackingTask(relay_parent);
    SL_TRACE(logger_,
             "Update my view.(new head={}, finalized={}, leaves={})",
             relay_parent,
             event.view.finalized_number_,
             event.view.heads_.size());
    broadcastView(event.view);
  }

  void ParachainProcessorImpl::onDeactivateBlocks(
      const primitives::events::ChainEventParams &event) {
    REINVOKE(*this_context_, onDeactivateBlocks, event);

    if (const auto value =
            if_type<const primitives::events::RemoveAfterFinalizationParams>(
                event)) {
      for (const auto &lost : value->get()) {
        SL_TRACE(logger_, "Remove from storages.(relay parent={})", lost);

        backing_store_->remove(lost);
        av_store_->remove(lost);
        bitfield_store_->remove(lost);
        our_current_state_.active_leaves.erase(lost);
      }
    }
  }

  void ParachainProcessorImpl::broadcastViewExcept(
      const libp2p::peer::PeerId &peer_id, const network::View &view) const {
    auto msg = std::make_shared<
        network::WireMessage<network::ValidatorProtocolMessage>>(
        network::ViewUpdate{.view = view});
    pm_->getStreamEngine()->broadcast(
        router_->getValidationProtocol(),
        msg,
        [&](const libp2p::peer::PeerId &p) { return peer_id != p; });
  }

  void ParachainProcessorImpl::broadcastView(const network::View &view) const {
    auto msg = std::make_shared<
        network::WireMessage<network::ValidatorProtocolMessage>>(
        network::ViewUpdate{.view = view});
    pm_->getStreamEngine()->broadcast(router_->getCollationProtocol(), msg);
    pm_->getStreamEngine()->broadcast(router_->getValidationProtocol(), msg);
  }

  outcome::result<std::optional<ValidatorSigner>>
  ParachainProcessorImpl::isParachainValidator(
      const primitives::BlockHash &relay_parent) const {
    return signer_factory_->at(relay_parent);
  }

  outcome::result<void> ParachainProcessorImpl::canProcessParachains() const {
    if (!isValidatingNode()) {
      return Error::NOT_A_VALIDATOR;
    }
    if (!babe_status_observer_->get()) {
      return Error::NOT_SYNCHRONIZED;
    }
    return outcome::success();
  }

  bool ParachainProcessorImpl::start() {
    in_pool_thread_handler_->start();
    this_context_->start();
    return true;
  }

  outcome::result<kagome::parachain::ParachainProcessorImpl::RelayParentState>
  ParachainProcessorImpl::initNewBackingTask(
      const primitives::BlockHash &relay_parent) {
    bool is_parachain_validator = false;
    auto metric_updater = gsl::finally([self{this}, &is_parachain_validator] {
      self->metric_is_parachain_validator_->set(is_parachain_validator);
    });
    OUTCOME_TRY(validators, parachain_host_->validators(relay_parent));
    OUTCOME_TRY(groups, parachain_host_->validator_groups(relay_parent));
    OUTCOME_TRY(cores, parachain_host_->availability_cores(relay_parent));
    OUTCOME_TRY(validator, isParachainValidator(relay_parent));
    auto &[validator_groups, group_rotation_info] = groups;

    if (!validator) {
      SL_TRACE(logger_, "Not a validator, or no para keys.");
      return Error::KEY_NOT_PRESENT;
    }
    is_parachain_validator = true;

    const auto n_cores = cores.size();
    std::optional<ParachainId> assignment;
    std::optional<CollatorId> required_collator;

    std::unordered_map<ParachainId, std::vector<ValidatorIndex>> out_groups;
    for (CoreIndex core_index = 0;
         core_index < static_cast<CoreIndex>(cores.size());
         ++core_index) {
      if (const auto *scheduled =
              boost::get<const network::ScheduledCore>(&cores[core_index])) {
        const auto group_index =
            group_rotation_info.groupForCore(core_index, n_cores);
        if (group_index < validator_groups.size()) {
          auto &g = validator_groups[group_index];
          if (g.contains(validator->validatorIndex())) {
            assignment = scheduled->para_id;
            required_collator = scheduled->collator;
          }
          out_groups[scheduled->para_id] = g.validators;
        }
      }
    }

    logger_->info(
        "Inited new backing task.(assignment={}, our index={}, relay "
        "parent={})",
        assignment,
        validator->validatorIndex(),
        relay_parent);

    return RelayParentState{
        .assignment = assignment,
        .seconded = {},
        .our_index = validator->validatorIndex(),
        .required_collator = required_collator,
        .table_context =
            TableContext{
                .validator = std::move(validator),
                .groups = std::move(out_groups),
                .validators = std::move(validators),
            },
        .awaiting_validation = {},
        .issued_statements = {},
        .peers_advertised = {},
        .fallbacks = {},
    };
  }

  void ParachainProcessorImpl::createBackingTask(
      const primitives::BlockHash &relay_parent) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    auto rps_result = initNewBackingTask(relay_parent);
    if (rps_result.has_value()) {
      storeStateByRelayParent(relay_parent, std::move(rps_result.value()));
    } else {
      logger_->error(
          "Relay parent state was not created. (relay parent={}, error={})",
          relay_parent,
          rps_result.error().message());
    }
  }

  void ParachainProcessorImpl::handleFetchedCollation(
      PendingCollation &&pending_collation,
      network::CollationFetchingResponse &&response) {
    REINVOKE(*this_context_,
             handleFetchedCollation,
             std::move(pending_collation),
             std::move(response));

    SL_TRACE(logger_,
             "Processing collation from {}, relay parent: {}, para id: {}",
             pending_collation.peer_id,
             pending_collation.relay_parent,
             pending_collation.para_id);

    our_current_state_.collation_requests_cancel_handles.erase(
        pending_collation);

    auto collation_response =
        if_type<network::CollationResponse>(response.response_data);
    if (!collation_response) {
      SL_WARN(logger_,
              "Not a CollationResponse message from {}.",
              pending_collation.peer_id);
      return;
    }

    auto opt_parachain_state =
        tryGetStateByRelayParent(pending_collation.relay_parent);
    if (!opt_parachain_state) {
      SL_TRACE(logger_,
               "Fetched collation from {}:{} out of view",
               pending_collation.peer_id,
               pending_collation.relay_parent);
      return;
    }

    auto &per_relay_parent = opt_parachain_state->get();
    auto &collations = per_relay_parent.collations;
    const auto &relay_parent_mode =
        per_relay_parent.prospective_parachains_mode;

    network::CandidateReceipt receipt(
        std::move(collation_response->get().receipt));
    network::ParachainBlock pov(std::move(collation_response->get().pov));
    const network::CandidateDescriptor &descriptor = receipt.descriptor;

    /// TODO(iceseer): do
    /// fetched_candidates ???

    auto &parachain_state = opt_parachain_state->get();
    auto &assignment = parachain_state.assignment;
    auto &seconded = parachain_state.seconded;
    auto &issued_statements = parachain_state.issued_statements;

    if (parachain_state.required_collator
        && *parachain_state.required_collator != descriptor.collator_id) {
      SL_WARN(logger_,
              "Fetched collation from wrong collator: received {} from {}",
              descriptor.collator_id,
              pending_collation.peer_id);
      return;
    }

    const auto candidate_para_id = descriptor.para_id;
    if (candidate_para_id != assignment) {
      SL_WARN(logger_,
              "Try to second for para_id {} out of our assignment {}.",
              candidate_para_id,
              assignment ? std::to_string(*assignment) : "{no assignment}");
      return;
    }

    //    if (seconded) {
    //      SL_DEBUG(logger_,
    //               "Already have seconded block {} instead of {}.",
    //               seconded->toString(),
    //               receipt.hash());
    //      return;
    //    }

    if (issued_statements.count(receipt.hash(*hasher_)) != 0) {
      SL_DEBUG(
          logger_, "Statement of {} already issued.", receipt.hash(*hasher_));
      return;
    }

    if (auto it = pending_candidates.find(pending_collation.relay_parent);
        it != pending_candidates.end()) {
      SL_WARN(logger_,
              "Trying to insert a pending candidate on {} failed, because "
              "there is already one.",
              pending_collation.relay_parent);
      return;
    }

    pending_collation.commitments_hash =
        collation_response->get().receipt.commitments_hash;

    runtime::PersistedValidationData pvd;
    if (relay_parent_mode && pending_collation.prospective_candidate) {
      pvd = requestProspectiveValidationData(
          pending_collation.relay_parent,
          pending_collation.prospective_candidate->second,
          pending_collation.para_id);
    } else if (!relay_parent_mode) {
      pvd = requestPersistedValidationData(receipt.descriptor.relay_parent,
                                           receipt.descriptor.para_id);
    } else {
      return;
    }

    /// TODO(iceseer): do
    /// fetched_collation_sanity_check

    pending_candidates.insert(
        std::make_pair(pending_collation.relay_parent, pending_collation));

    collations.status = CollationStatus::WaitingOnValidation;
    validateAsync<ValidationTaskType::kSecond>(
        std::move(collation_response->get().receipt),
        std::move(collation_response->get().pov),
        std::move(pvd),
        pending_collation.peer_id,
        pending_collation.relay_parent,
        parachain_state.table_context.validators.size());
  }

  std::optional<runtime::PersistedValidationData>
  ParachainProcessorImpl::requestProspectiveValidationData(
      const RelayHash &relay_parent,
      const Hash &parent_head_data_hash,
      ParachainId para_id) {
    return prospective_parachains_->answerProspectiveValidationDataRequest(
        relay_parent, parent_head_data_hash, para_id);
  }

  std::optional<runtime::PersistedValidationData>
  ParachainProcessorImpl::requestPersistedValidationData(
      const RelayHash &relay_parent, ParachainId para_id) {
    auto res_data = parachain_api_->persisted_validation_data(
        relay_parent, para_id, runtime::OccupiedCoreAssumption::Free);
    if (res_data.has_error()) {
      SL_VERBOSE(log_,
                 "PersistedValidationData not found. (error={}, "
                 "relay_parent={} para_id={})",
                 res_data.error().message(),
                 relay_parent,
                 para_id);
      return std::nullopt;
    }
    return std::move(res_data.value());
  }

  void ParachainProcessorImpl::processIncomingPeerMessage(
      const libp2p::peer::PeerId &peer_id,
      network::VersionedCollatorProtocolMessage &&msg) {
    REINVOKE(
        *this_context_, processIncomingPeerMessage, peer_id, std::move(msg));
  }

  void ParachainProcessorImpl::onValidationProtocolMsg(
      const libp2p::peer::PeerId &peer_id,
      const network::ValidatorProtocolMessage &message) {
    if (auto m{boost::get<network::BitfieldDistributionMessage>(&message)}) {
      auto bd{boost::get<network::BitfieldDistribution>(m)};
      BOOST_ASSERT_MSG(
          bd, "BitfieldDistribution is not present. Check message format.");

      SL_TRACE(logger_,
               "Imported bitfield {} {}",
               bd->data.payload.ix,
               bd->relay_parent);
      bitfield_store_->putBitfield(bd->relay_parent, bd->data);
      return;
    }

    if (auto msg{boost::get<network::StatementDistributionMessage>(&message)}) {
      if (auto statement_msg{boost::get<network::Seconded>(msg)}) {
        if (auto r = canProcessParachains(); r.has_error()) {
          return;
        }
        if (auto r = isParachainValidator(statement_msg->relay_parent);
            r.has_error() || !r.value()) {
          return;
        }
        SL_TRACE(
            logger_, "Imported statement on {}", statement_msg->relay_parent);
        handleStatement(
            peer_id, statement_msg->relay_parent, statement_msg->statement);
      } else {
        auto &large = boost::get<network::LargeStatement>(*msg);
        // TODO(turuslan): #1757, LargeStatement
        SL_ERROR(logger_,
                 "Ignoring LargeStatement about {} from {}",
                 large.payload.payload.candidate_hash,
                 peer_id);
      }
      return;
    }
  }

  template <typename F>
  void ParachainProcessorImpl::requestPoV(
      const libp2p::peer::PeerInfo &peer_info,
      const CandidateHash &candidate_hash,
      F &&callback) {
    /// TODO(iceseer): request PoV from validator, who seconded candidate
    /// But now we can assume, that if we received either `seconded` or `valid`
    /// from some peer, than we expect this peer has valid PoV, which we can
    /// request.

    logger_->info("Requesting PoV.(candidate hash={}, peer={})",
                  candidate_hash,
                  peer_info.id);

    auto protocol = router_->getReqPovProtocol();
    BOOST_ASSERT(protocol);

    protocol->request(peer_info, candidate_hash, std::forward<F>(callback));
  }

  std::optional<runtime::SessionInfo>
  ParachainProcessorImpl::retrieveSessionInfo(const RelayHash &relay_parent) {
    if (auto session_index =
            parachain_host_->session_index_for_child(relay_parent);
        session_index.has_value()) {
      if (auto session_info = parachain_host_->session_info(
              relay_parent, session_index.value());
          session_info.has_value()) {
        return session_info.value();
      }
    }
    return std::nullopt;
  }

  void ParachainProcessorImpl::kickOffValidationWork(
      const RelayHash &relay_parent,
      AttestingData &attesting_data,
      RelayParentState &parachain_state) {
    const auto candidate_hash{attesting_data.candidate.hash(*hasher_)};

    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    if (!parachain_state.awaiting_validation.insert(candidate_hash).second) {
      return;
    }

    const auto &collator_id =
        collatorIdFromDescriptor(attesting_data.candidate.descriptor);
    if (parachain_state.required_collator
        && collator_id != *parachain_state.required_collator) {
      parachain_state.issued_statements.insert(candidate_hash);
      return;
    }

    auto session_info = retrieveSessionInfo(relay_parent);
    if (!session_info) {
      SL_WARN(logger_, "No session info.(relay_parent={})", relay_parent);
      return;
    }

    if (session_info->discovery_keys.size() <= attesting_data.from_validator) {
      SL_ERROR(logger_,
               "Invalid validator index.(relay_parent={}, validator_index={})",
               relay_parent,
               attesting_data.from_validator);
      return;
    }

    const auto &authority_id =
        session_info->discovery_keys[attesting_data.from_validator];
    if (auto peer = query_audi_->get(authority_id)) {
      requestPoV(
          *peer,
          candidate_hash,
          [candidate{attesting_data.candidate},
           candidate_hash,
           wself{weak_from_this()},
           relay_parent,
           peer_id{peer->id}](auto &&pov_response_result) mutable {
            if (auto self = wself.lock()) {
              auto parachain_state =
                  self->tryGetStateByRelayParent(relay_parent);
              if (!parachain_state) {
                self->logger_->warn(
                    "After request pov no parachain state on relay_parent {}",
                    relay_parent);
                return;
              }

              if (!pov_response_result) {
                self->logger_->warn("Request PoV on relay_parent {} failed {}",
                                    relay_parent,
                                    pov_response_result.error().message());
                return;
              }

              network::ResponsePov &opt_pov = pov_response_result.value();
              auto p{boost::get<network::ParachainBlock>(&opt_pov)};
              if (!p) {
                self->logger_->warn("No PoV.(candidate={})", candidate_hash);
                self->onAttestNoPoVComplete(relay_parent, candidate_hash);
                return;
              }

              self->logger_->info(
                  "PoV received.(relay_parent={}, candidate hash={}, peer={})",
                  relay_parent,
                  candidate_hash,
                  peer_id);
              self->validateAsync<ValidationTaskType::kAttest>(
                  std::move(candidate),
                  std::move(*p),
                  peer_id,
                  relay_parent,
                  parachain_state->get().table_context.validators.size());
            }
          });
    }
  }

  outcome::result<network::FetchChunkResponse>
  ParachainProcessorImpl::OnFetchChunkRequest(
      const network::FetchChunkRequest &request) {
    if (auto chunk =
            av_store_->getChunk(request.candidate, request.chunk_index)) {
      return network::Chunk{
          .data = chunk->chunk,
          .proof = chunk->proof,
      };
    }
    return network::FetchChunkResponse{};
  }

  std::optional<
      std::reference_wrapper<ParachainProcessorImpl::RelayParentState>>
  ParachainProcessorImpl::tryGetStateByRelayParent(
      const primitives::BlockHash &relay_parent) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    const auto it = our_current_state_.state_by_relay_parent.find(relay_parent);
    if (it != our_current_state_.state_by_relay_parent.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  ParachainProcessorImpl::RelayParentState &
  ParachainProcessorImpl::storeStateByRelayParent(
      const primitives::BlockHash &relay_parent, RelayParentState &&val) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    const auto &[it, inserted] =
        our_current_state_.state_by_relay_parent.insert(
            {relay_parent, std::move(val)});
    BOOST_ASSERT(inserted);
    return it->second;
  }

  void ParachainProcessorImpl::handleStatement(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &relay_parent,
      const network::SignedStatement &statement) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    auto opt_parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!opt_parachain_state) {
      logger_->trace(
          "Handled statement from {}:{} out of view", peer_id, relay_parent);
      return;
    }

    auto &parachain_state = opt_parachain_state->get();
    auto &assignment = parachain_state.assignment;
    auto &fallbacks = parachain_state.fallbacks;
    auto &awaiting_validation = parachain_state.awaiting_validation;

    if (auto result =
            importStatement(relay_parent, statement, parachain_state)) {
      if (result->imported.group_id != assignment) {
        SL_TRACE(
            logger_,
            "Registered statement from not our group(our: {}, registered: {}).",
            assignment,
            result->imported.group_id);
        return;
      }

      SL_TRACE(logger_,
               "Registered incoming statement.(relay_parent={}, peer={}).",
               relay_parent,
               peer_id);
      std::optional<std::reference_wrapper<AttestingData>> attesting_ref =
          visit_in_place(
              parachain::getPayload(statement).candidate_state,
              [&](const network::CommittedCandidateReceipt &seconded)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto const &candidate_hash = result->imported.candidate;
                auto opt_candidate =
                    backing_store_->get_candidate(candidate_hash);
                if (!opt_candidate) {
                  logger_->error("No candidate {}", candidate_hash);
                  return std::nullopt;
                }

                AttestingData attesting{
                    .candidate =
                        candidateFromCommittedCandidateReceipt(*opt_candidate),
                    .pov_hash = seconded.descriptor.pov_hash,
                    .from_validator = statement.payload.ix,
                    .backing = {}};

                auto const &[it, _] = fallbacks.insert(
                    std::make_pair(candidate_hash, std::move(attesting)));
                return it->second;
              },
              [&](const primitives::BlockHash &candidate_hash)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto it = fallbacks.find(candidate_hash);
                if (it == fallbacks.end()) {
                  return std::nullopt;
                }
                if (!parachain_state.our_index
                    || *parachain_state.our_index == statement.payload.ix) {
                  return std::nullopt;
                }
                if (awaiting_validation.count(candidate_hash) > 0) {
                  it->second.backing.push(statement.payload.ix);
                  return std::nullopt;
                }
                it->second.from_validator = statement.payload.ix;
                return it->second;
              },
              [&](const auto &)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                BOOST_ASSERT(!"Not used!");
                return std::nullopt;
              });

      if (attesting_ref) {
        kickOffValidationWork(
            relay_parent, attesting_ref->get(), parachain_state);
      }
    }
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatementToTable(
      ParachainProcessorImpl::RelayParentState &relayParentState,
      const primitives::BlockHash &candidate_hash,
      const network::SignedStatement &statement) {
    SL_TRACE(
        logger_, "Import statement into table.(candidate={})", candidate_hash);

    if (auto r = backing_store_->put(relayParentState.table_context.groups,
                                     statement)) {
      return ImportStatementSummary{
          .imported = *r,
          .attested = false,
      };
    }
    return std::nullopt;
  }

  void ParachainProcessorImpl::notifyBackedCandidate(
      const network::SignedStatement &statement) {
    logger_->error(
        "Not implemented. Should notify somebody that backed candidate "
        "appeared.");
  }

  std::optional<ParachainProcessorImpl::AttestedCandidate>
  ParachainProcessorImpl::attested(
      network::CommittedCandidateReceipt &&candidate,
      const BackingStore::StatementInfo &data,
      size_t validity_threshold) {
    const auto &validity_votes = data.second;
    const auto valid_votes = validity_votes.size();
    if (valid_votes < validity_threshold) {
      return std::nullopt;
    }

    std::vector<std::pair<ValidatorIndex, network::ValidityAttestation>>
        validity_votes_out;
    validity_votes_out.reserve(validity_votes.size());

    for (auto &[validator_index, validity_vote] : validity_votes) {
      auto validity_attestation = visit_in_place(
          validity_vote,
          [](const BackingStore::ValidityVoteIssued &val) {
            return network::ValidityAttestation{
                network::ValidityAttestation::Implicit{},
                ((BackingStore::Statement &)val).signature,
            };
          },
          [](const BackingStore::ValidityVoteValid &val) {
            return network::ValidityAttestation{
                network::ValidityAttestation::Explicit{},
                ((BackingStore::Statement &)val).signature,
            };
          });

      validity_votes_out.emplace_back(validator_index,
                                      std::move(validity_attestation));
    }

    return AttestedCandidate{
        .group_id = data.first,
        .candidate = std::move(candidate),
        .validity_votes = std::move(validity_votes_out),
    };
  }

  std::optional<ParachainProcessorImpl::AttestedCandidate>
  ParachainProcessorImpl::attested_candidate(
      const CandidateHash &digest,
      const ParachainProcessorImpl::TableContext &context) {
    if (auto opt_validity_votes = backing_store_->get_validity_votes(digest)) {
      auto &data = opt_validity_votes->get();
      const GroupIndex group = data.first;

      auto candidate{backing_store_->get_candidate(digest)};
      BOOST_ASSERT(candidate);

      const auto v_threshold = context.requisite_votes(group);
      return attested(std::move(*candidate), data, v_threshold);
    }
    return std::nullopt;
  }

  std::optional<BackingStore::BackedCandidate>
  ParachainProcessorImpl::table_attested_to_backed(
      AttestedCandidate &&attested, TableContext &table_context) {
    const auto para_id = attested.group_id;
    if (auto it = table_context.groups.find(para_id);
        it != table_context.groups.end()) {
      const auto &group = it->second;
      scale::BitVec validator_indices{};
      validator_indices.bits.resize(group.size(), false);

      std::vector<std::pair<size_t, size_t>> vote_positions;
      vote_positions.reserve(attested.validity_votes.size());

      auto position = [](const auto &container,
                         const auto &val) -> std::optional<size_t> {
        for (size_t ix = 0; ix < container.size(); ++ix) {
          if (val == container[ix]) {
            return ix;
          }
        }
        return std::nullopt;
      };

      for (size_t orig_idx = 0; orig_idx < attested.validity_votes.size();
           ++orig_idx) {
        const auto &id = attested.validity_votes[orig_idx].first;
        if (auto p = position(group, id)) {
          validator_indices.bits[*p] = true;
          vote_positions.emplace_back(orig_idx, *p);
        } else {
          logger_->critical(
              "Logic error: Validity vote from table does not correspond to "
              "group.");
          return std::nullopt;
        }
      }
      std::sort(
          vote_positions.begin(),
          vote_positions.end(),
          [](const auto &l, const auto &r) { return l.second < r.second; });

      std::vector<network::ValidityAttestation> validity_votes;
      validity_votes.reserve(vote_positions.size());
      for (const auto &[pos_in_votes, _pos_in_group] : vote_positions) {
        validity_votes.emplace_back(
            std::move(attested.validity_votes[pos_in_votes].second));
      }

      return BackingStore::BackedCandidate{
          .candidate = std::move(attested.candidate),
          .validity_votes = std::move(validity_votes),
          .validator_indices = std::move(validator_indices),
      };
    }
    return std::nullopt;
  }

  std::optional<ParachainProcessorImpl::ImportStatementSummary>
  ParachainProcessorImpl::importStatement(
      const network::RelayHash &relay_parent,
      const network::SignedStatement &statement,
      ParachainProcessorImpl::RelayParentState &relayParentState) {
    auto import_result = importStatementToTable(
        relayParentState,
        candidateHashFrom(parachain::getPayload(statement)),
        statement);

    if (import_result) {
      SL_TRACE(logger_,
               "Import result.(candidate={}, group id={}, validity votes={})",
               import_result->imported.candidate,
               import_result->imported.group_id,
               import_result->imported.validity_votes);

      if (auto attested = attested_candidate(import_result->imported.candidate,
                                             relayParentState.table_context)) {
        if (relayParentState.backed_hashes
                .insert(candidateHash(*hasher_, attested->candidate))
                .second) {
          if (auto backed = table_attested_to_backed(
                  std::move(*attested), relayParentState.table_context)) {
            SL_INFO(
                logger_,
                "Candidate backed.(candidate={}, para id={}, relay_parent={})",
                import_result->imported.candidate,
                import_result->imported.group_id,
                relay_parent);
            backing_store_->add(relay_parent, std::move(*backed));
          }
        }
      }
    }

    if (import_result && import_result->attested) {
      notifyBackedCandidate(statement);
    }
    return import_result;
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  std::optional<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatement(
      ValidateAndSecondResult &validation_result) {
    static_assert(kStatementType == StatementType::kSeconded
                  || kStatementType == StatementType::kValid);

    auto parachain_state =
        tryGetStateByRelayParent(validation_result.relay_parent);
    if (!parachain_state) {
      logger_->error("Create and sign statement. No such relay_parent {}.",
                     validation_result.relay_parent);
      return std::nullopt;
    }

    if (!parachain_state->get().our_index) {
      logger_->template warn(
          "We are not validators or we have no validator index.");
      return std::nullopt;
    }

    if constexpr (kStatementType == StatementType::kSeconded) {
      return createAndSignStatementFromPayload(
          network::Statement{
              .candidate_state =
                  network::CommittedCandidateReceipt{
                      .descriptor = validation_result.candidate.descriptor,
                      .commitments =
                          std::move(*validation_result.commitments)}},
          *parachain_state->get().our_index,
          parachain_state->get());
    } else if constexpr (kStatementType == StatementType::kValid) {
      return createAndSignStatementFromPayload(
          network::Statement{.candidate_state =
                                 validation_result.candidate.hash(*hasher_)},
          *parachain_state->get().our_index,
          parachain_state->get());
    }
  }

  template <typename T>
  std::optional<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatementFromPayload(
      T &&payload,
      ValidatorIndex validator_ix,
      RelayParentState &parachain_state) {
    /// TODO(iceseer):
    /// https://github.com/paritytech/polkadot/blob/master/primitives/src/v2/mod.rs#L1535-L1545
    auto sign_result =
        parachain_state.table_context.validator->sign(std::move(payload));
    if (sign_result.has_error()) {
      logger_->error(
          "Unable to sign Commited Candidate Receipt. Failed with error: {}",
          sign_result.error().message());
      return std::nullopt;
    }

    return sign_result.value();
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingStream(
      const libp2p::peer::PeerId &peer_id,
      std::shared_ptr<network::ProtocolBase> protocol,
      F &&callback) {
    auto stream_engine = pm_->getStreamEngine();
    BOOST_ASSERT(stream_engine);

    if (stream_engine->reserveOutgoing(peer_id, protocol)) {
      protocol->newOutgoingStream(
          libp2p::peer::PeerInfo{.id = peer_id, .addresses = {}},
          [callback{std::forward<F>(callback)},
           protocol,
           peer_id,
           wptr{weak_from_this()}](auto &&stream_result) mutable {
            auto self = wptr.lock();
            if (not self) {
              return;
            }

            auto stream_engine = self->pm_->getStreamEngine();
            stream_engine->dropReserveOutgoing(peer_id, protocol);

            if (!stream_result.has_value()) {
              self->logger_->warn("Unable to create stream {} with {}: {}",
                                  protocol->protocolName(),
                                  peer_id,
                                  stream_result.error());
              return;
            }

            auto stream = stream_result.value();
            if (auto add_result = stream_engine->addOutgoing(
                    std::move(stream_result.value()), protocol);
                !add_result) {
              self->logger_->error("Unable to store stream {} with {}: {}",
                                   protocol->protocolName(),
                                   peer_id,
                                   add_result.error().message());
              return;
            }

            std::forward<F>(callback)(std::move(stream));
          });
      return true;
    }
    return false;
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingCollatingStream(
      const libp2p::peer::PeerId &peer_id, F &&callback) {
    auto protocol = router_->getCollationProtocol();
    BOOST_ASSERT(protocol);

    return tryOpenOutgoingStream(
        peer_id, std::move(protocol), std::forward<F>(callback));
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingValidationStream(
      const libp2p::peer::PeerId &peer_id, F &&callback) {
    auto protocol = router_->getValidationProtocol();
    BOOST_ASSERT(protocol);

    return tryOpenOutgoingStream(
        peer_id, std::move(protocol), std::forward<F>(callback));
  }

  void ParachainProcessorImpl::sendMyView(
      const libp2p::peer::PeerId &peer_id,
      const std::shared_ptr<network::Stream> &stream,
      const std::shared_ptr<network::ProtocolBase> &protocol) {
    auto my_view = peer_view_->getMyView();
    if (!my_view) {
      logger_->error("sendMyView failed, because my view still is not exists.");
      return;
    }

    BOOST_ASSERT(protocol);
    logger_->info("Send my view.(peer={}, protocol={})",
                  peer_id,
                  protocol->protocolName());
    pm_->getStreamEngine()->template send(
        peer_id,
        protocol,
        std::make_shared<
            network::WireMessage<network::ValidatorProtocolMessage>>(
            network::ViewUpdate{.view = my_view->get().view}));
  }

  void ParachainProcessorImpl::onIncomingCollationStream(
      const libp2p::peer::PeerId &peer_id) {
    if (tryOpenOutgoingCollatingStream(
            peer_id, [wptr{weak_from_this()}, peer_id](auto &&stream) {
              if (auto self = wptr.lock()) {
                self->sendMyView(
                    peer_id, stream, self->router_->getCollationProtocol());
              }
            })) {
      SL_DEBUG(logger_, "Initiated collation protocol with {}", peer_id);
    }
  }

  void ParachainProcessorImpl::onIncomingValidationStream(
      const libp2p::peer::PeerId &peer_id) {
    if (tryOpenOutgoingValidationStream(
            peer_id, [wptr{weak_from_this()}, peer_id](auto &&stream) {
              if (auto self = wptr.lock()) {
                self->sendMyView(
                    peer_id, stream, self->router_->getValidationProtocol());
              }
            })) {
      logger_->info("Initiated validation protocol with {}", peer_id);
    }
  }

  network::ResponsePov ParachainProcessorImpl::getPov(
      CandidateHash &&candidate_hash) {
    if (auto res = av_store_->getPov(candidate_hash)) {
      return network::ResponsePov{*res};
    }
    return network::Empty{};
  }

  void ParachainProcessorImpl::onIncomingCollator(
      const libp2p::peer::PeerId &peer_id,
      network::CollatorPublicKey pubkey,
      network::ParachainId para_id) {
    pm_->setCollating(peer_id, pubkey, para_id);
  }

  void ParachainProcessorImpl::handleNotify(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &relay_parent) {
    if (tryOpenOutgoingCollatingStream(
            peer_id,
            [peer_id, relay_parent, wptr{weak_from_this()}](
                auto && /*stream*/) {
              auto self = wptr.lock();
              if (not self) {
                return;
              }
              self->handleNotify(peer_id, relay_parent);
            })) {
      return;
    }

    logger_->info("Send Seconded to collator.(peer={}, relay parent={})",
                  peer_id,
                  relay_parent);

    auto stream_engine = pm_->getStreamEngine();
    BOOST_ASSERT(stream_engine);

    auto collation_protocol = router_->getCollationProtocol();
    BOOST_ASSERT(collation_protocol);

    auto &statements_queue = our_current_state_.seconded_statements[peer_id];
    while (!statements_queue.empty()) {
      auto p{std::move(statements_queue.front())};
      statements_queue.pop_front();
      network::SignedStatement &statement = p.second;
      RelayHash &rp = p.first;

      pending_candidates.erase(rp);
      stream_engine->send(
          peer_id,
          collation_protocol,
          std::make_shared<
              network::WireMessage<network::CollationProtocolMessage>>(
              network::CollationProtocolMessage(
                  network::CollationMessage(network::Seconded{
                      .relay_parent = rp, .statement = statement}))));
    }
  }

  void ParachainProcessorImpl::notify(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &relay_parent,
      const network::SignedStatement &statement) {
    our_current_state_.seconded_statements[peer_id].emplace_back(
        std::make_pair(relay_parent, statement));
    handleNotify(peer_id, relay_parent);
  }

  bool ParachainProcessorImpl::isValidatingNode() const {
    return (app_config_.roles().flags.authority == 1);
  }

  outcome::result<void> ParachainProcessorImpl::advCanBeProcessed(
      const primitives::BlockHash &relay_parent,
      const libp2p::peer::PeerId &peer_id) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());
    OUTCOME_TRY(canProcessParachains());

    auto rps = our_current_state_.state_by_relay_parent.find(relay_parent);
    if (rps == our_current_state_.state_by_relay_parent.end()) {
      return Error::OUT_OF_VIEW;
    }

    if (rps->second.peers_advertised.count(peer_id) != 0ull) {
      return Error::DUPLICATE;
    }

    rps->second.peers_advertised.insert(peer_id);
    return outcome::success();
  }

  void ParachainProcessorImpl::onValidationComplete(
      const libp2p::peer::PeerId &peer_id,
      ValidateAndSecondResult &&validation_result) {
    logger_->trace("On validation complete. (peer={}, relay parent={})",
                   peer_id,
                   validation_result.relay_parent);

    auto opt_parachain_state =
        tryGetStateByRelayParent(validation_result.relay_parent);
    if (!opt_parachain_state) {
      logger_->trace("Validated candidate from {}:{} out of view",
                     peer_id,
                     validation_result.relay_parent);
      return;
    }

    auto &parachain_state = opt_parachain_state->get();
    auto &seconded = parachain_state.seconded;
    const auto candidate_hash = validation_result.candidate.hash(*hasher_);
    if (!validation_result.result) {
      logger_->warn("Candidate {} validation failed with: {}",
                    candidate_hash,
                    validation_result.result.error());
      // send invalid
    } else if (!seconded
               && parachain_state.issued_statements.count(candidate_hash)
                      == 0) {
      logger_->trace(
          "Second candidate complete. (candidate={}, peer={}, relay parent={})",
          candidate_hash,
          peer_id,
          validation_result.relay_parent);

      seconded = candidate_hash;
      parachain_state.issued_statements.insert(candidate_hash);
      if (auto statement = createAndSignStatement<StatementType::kSeconded>(
              validation_result)) {
        importStatement(
            validation_result.relay_parent, *statement, parachain_state);
        notifyStatementDistributionSystem(validation_result.relay_parent,
                                          *statement);
        notify(peer_id, validation_result.relay_parent, *statement);
      }
    }
  }

  void ParachainProcessorImpl::notifyStatementDistributionSystem(
      const primitives::BlockHash &relay_parent,
      const network::SignedStatement &statement) {
    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    // TODO(turuslan): #1757, LargeStatement
    auto message = std::make_shared<
        network::WireMessage<network::ValidatorProtocolMessage>>(
        network::ValidatorProtocolMessage{
            network::StatementDistributionMessage{network::Seconded{
                .relay_parent = relay_parent, .statement = statement}}});

    // Send to all peers in our group.
    // If our group is smaller than `kMinGossipPeers`
    // select other peers randomly.
    std::unordered_set<network::PeerId> group_set;
    if (auto r = runtime_info_->get_session_info(relay_parent)) {
      auto &[session, info] = r.value();
      if (info.our_group) {
        for (auto &i : session.validator_groups[*info.our_group]) {
          if (auto peer = query_audi_->get(session.discovery_keys[i])) {
            group_set.emplace(peer->id);
          }
        }
      }
    }
    std::deque<network::PeerId> group, any;
    auto protocol = router_->getValidationProtocol();
    se->forEachPeer(protocol, [&](const network::PeerId &peer) {
      (group_set.count(peer) != 0 ? group : any).emplace_back(peer);
    });
    auto lucky = kMinGossipPeers - std::min(group.size(), kMinGossipPeers);
    if (lucky != 0) {
      std::shuffle(any.begin(), any.end(), random_);
      any.erase(any.begin() + std::min(any.size(), lucky), any.end());
    } else {
      any.clear();
    }

    logger_->trace(
        "Broadcasting StatementDistributionMessage.(relay_parent={}, validator "
        "index={}, sig={})",
        relay_parent,
        statement.payload.ix,
        statement.signature);

    auto send = [&](const network::PeerId &peer) {
      se->send(peer, protocol, message);
    };
    for (auto &peer : group) {
      send(peer);
    }
    for (auto &peer : any) {
      send(peer);
    }
  }

  outcome::result<kagome::parachain::Pvf::Result>
  ParachainProcessorImpl::validateCandidate(
      const network::CandidateReceipt &candidate,
      const network::ParachainBlock &pov,
      const primitives::BlockHash &relay_parent) {
    return pvf_->pvfSync(candidate, pov);
  }

  outcome::result<std::vector<network::ErasureChunk>>
  ParachainProcessorImpl::validateErasureCoding(
      const runtime::AvailableData &validating_data, size_t n_validators) {
    return toChunks(n_validators, validating_data);
  }

  void ParachainProcessorImpl::notifyAvailableData(
      std::vector<network::ErasureChunk> &&chunks,
      const primitives::BlockHash &relay_parent,
      const network::CandidateHash &candidate_hash,
      const network::ParachainBlock &pov,
      const runtime::PersistedValidationData &data) {
    makeTrieProof(chunks);
    /// TODO(iceseer): remove copy
    av_store_->storeData(
        relay_parent, candidate_hash, std::move(chunks), pov, data);
    logger_->trace("Put chunks set.(candidate={})", candidate_hash);
  }

  template <ParachainProcessorImpl::ValidationTaskType kMode>
  void ParachainProcessorImpl::makeAvailable(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &candidate_hash,
      ValidateAndSecondResult &&validate_and_second_result) {
    REINVOKE(*this_context_,
             makeAvailable<kMode>,
             peer_id,
             candidate_hash,
             std::move(validate_and_second_result));

    auto parachain_state =
        tryGetStateByRelayParent(validate_and_second_result.relay_parent);
    if (!parachain_state) {
      SL_WARN(logger_,
              "After validation no parachain state on relay_parent {}",
              validate_and_second_result.relay_parent);
      return;
    }

    SL_INFO(logger_,
            "Async validation complete.(relay parent={}, para_id={})",
            validate_and_second_result.relay_parent,
            validate_and_second_result.candidate.descriptor.para_id);

    parachain_state->get().awaiting_validation.erase(candidate_hash);
    auto q{std::move(validate_and_second_result)};
    if constexpr (kMode == ValidationTaskType::kSecond) {
      onValidationComplete(peer_id, std::move(q));
    } else {
      onAttestComplete(peer_id, std::move(q));
    }
  }

  template <ParachainProcessorImpl::ValidationTaskType kMode>
  void ParachainProcessorImpl::validateAsync(
      network::CandidateReceipt &&candidate,
      network::ParachainBlock &&pov,
      runtime::PersistedValidationData &&pvd,
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &relay_parent,
      size_t n_validators) {
    REINVOKE(*in_pool_thread_handler_,
             validateAsync<kMode>,
             std::move(candidate),
             std::move(pov),
             std::move(pvd),
             peer_id,
             relay_parent,
             n_validators);

    SL_INFO(logger_,
            "Starting validation task.(para id={}, "
            "relay parent={}, peer={})",
            candidate.descriptor.para_id,
            relay_parent,
            peer_id);

    TicToc _measure{"Parachain validation", logger_};
    const auto candidate_hash{candidate.hash(*hasher_)};

    /// checks if we still need to execute parachain task
    /// TODO(iceseer): do
    auto need_to_process =
        our_current_state_.active_leaves.count(relay_parent) != 0ull;

    if (!need_to_process) {
      SL_TRACE(logger_,
               "Candidate validation skipped because of extruded relay parent. "
               "(relay_parent={}, parachain_id={}, candidate_hash={})",
               relay_parent,
               candidate.descriptor.para_id,
               candidate_hash);
      return;
    }

    auto validation_result = validateCandidate(candidate, pov, relay_parent);
    if (!validation_result) {
      logger_->warn(
          "Candidate {} on relay_parent {}, para_id {} validation failed with "
          "error: {}",
          candidate_hash,
          candidate.descriptor.relay_parent,
          candidate.descriptor.para_id,
          validation_result.error().message());
      return;
    }

    /// TODO(iceseer): do
    need_to_process =
        our_current_state_.active_leaves.count(relay_parent) != 0ull;

    if (!need_to_process) {
      SL_TRACE(logger_,
               "Candidate validation skipped before erasure-coding because of "
               "extruded relay parent. "
               "(relay_parent={}, parachain_id={}, candidate_hash={})",
               relay_parent,
               candidate.descriptor.para_id,
               candidate_hash);
      return;
    }

    auto &[comms, data] = validation_result.value();
    runtime::AvailableData available_data{
        .pov = std::move(pov),
        .validation_data = std::move(data),
    };

    std::vector<network::ErasureChunk> chunks;
    if (auto res = validateErasureCoding(available_data, n_validators);
        res.has_error()) {
      SL_WARN(logger_,
              "Erasure coding validation failed. (error={})",
              res.error().message());
      return;
    } else {
      chunks = std::move(res.value());
    }

    notifyAvailableData(std::move(chunks),
                        relay_parent,
                        candidate_hash,
                        available_data.pov,
                        available_data.validation_data);

    makeAvailable<kMode>(
        peer_id,
        candidate_hash,
        ValidateAndSecondResult{
            .result = outcome::success(),
            .relay_parent = relay_parent,
            .commitments = std::make_shared<network::CandidateCommitments>(
                std::move(comms)),
            .candidate = std::move(candidate),
            .pov = std::move(available_data.pov),
        });
  }

  void ParachainProcessorImpl::onAttestComplete(
      const libp2p::peer::PeerId &, ValidateAndSecondResult &&result) {
    auto parachain_state = tryGetStateByRelayParent(result.relay_parent);
    if (!parachain_state) {
      logger_->warn(
          "onAttestComplete result based on unexpected relay_parent {}",
          result.relay_parent);
      return;
    }

    logger_->info("Attest complete.(relay parent={}, para id={})",
                  result.relay_parent,
                  result.candidate.descriptor.para_id);

    const auto candidate_hash = result.candidate.hash(*hasher_);
    parachain_state->get().fallbacks.erase(candidate_hash);

    if (parachain_state->get().issued_statements.count(candidate_hash) == 0) {
      if (result.result) {
        if (auto statement =
                createAndSignStatement<StatementType::kValid>(result)) {
          importStatement(
              result.relay_parent, *statement, parachain_state->get());
          notifyStatementDistributionSystem(result.relay_parent, *statement);
        }
      }
      parachain_state->get().issued_statements.insert(candidate_hash);
    }
  }

  void ParachainProcessorImpl::onAttestNoPoVComplete(
      const network::RelayHash &relay_parent,
      const CandidateHash &candidate_hash) {
    auto parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!parachain_state) {
      logger_->warn(
          "onAttestNoPoVComplete result based on unexpected relay_parent. "
          "(relay_parent={}, candidate={})",
          relay_parent,
          candidate_hash);
      return;
    }

    auto it = parachain_state->get().fallbacks.find(candidate_hash);
    if (it == parachain_state->get().fallbacks.end()) {
      logger_->error(
          "Internal error. Fallbacks doesn't contain candidate hash {}",
          candidate_hash);
      return;
    }

    /// TODO(iceseer): make rotation on validators
    AttestingData &attesting = it->second;
    if (!attesting.backing.empty()) {
      attesting.from_validator = attesting.backing.front();
      attesting.backing.pop();
      kickOffValidationWork(relay_parent, attesting, *parachain_state);
    }
  }

  bool ParachainProcessorImpl::isRelayParentInImplicitView(
      const RelayHash &relay_parent,
      const ProspectiveParachainsModeOpt &relay_parent_mode,
      const ImplicitView &implicit_view,
      const std::unordered_map<Hash, ProspectiveParachainsModeOpt>
          &active_leaves,
      ParachainId para_id) {
    if (!relay_parent_mode) {
      return active_leaves.count(relay_parent) != 0ull;
    }

    for (const auto &[hash, mode] : active_leaves) {
      if (mode) {
        for (const auto &h :
             implicit_view.knownAllowedRelayParentsUnder(hash, para_id)) {
          if (h == relay_parent) {
            return true;
          }
        }
      }
    }
    return false;
  }

  outcome::result<std::pair<CollatorId, ParachainId>>
  ParachainProcessorImpl::insertAdvertisement(
      network::PeerState &peer_data,
      const RelayHash &on_relay_parent,
      const ProspectiveParachainsModeOpt &relay_parent_mode,
      const std::optional<std::reference_wrapper<const CandidateHash>>
          &candidate_hash) {
    if (!peer_data.collator_state) {
      SL_WARN(logger_, "Undeclared collator.");
      return Error::UNDECLARED_COLLATOR;
    }

    if (!isRelayParentInImplicitView(on_relay_parent,
                                     relay_parent_mode,
                                     our_current_state_.implicit_view,
                                     our_current_state_.active_leaves,
                                     peer_data.collator_state->para_id)) {
      return Error::OUT_OF_VIEW;
    }

    if (!relay_parent_mode) {
      if (peer_data.collator_state->advertisements.count(on_relay_parent)
          != 0ull) {
        return Error::DUPLICATE;
      }

      if (candidate_hash) {
        peer_data.collator_state->advertisements[on_relay_parent] = {
            *candidate_hash};
      }
    } else if (candidate_hash) {
      auto &candidates =
          peer_data.collator_state->advertisements[on_relay_parent];
      auto [it, inserted] = candidates.insert(*candidate_hash);
      if (!inserted) {
        return Error::DUPLICATE;
      }
      if (candidates.size() > relay_parent_mode->max_candidate_depth) {
        candidates.erase(*candidate_hash);
        return Error::PEER_LIMIT_REACHED;
      }
    } else {
      return Error::PROTOCOL_MISMATCH;
    }

    peer_data.collator_state->last_active = std::chrono::system_clock::now();
    return std::make_pair(peer_data.collator_state->collator_id,
                          peer_data.collator_state->para_id);
  }

  ParachainProcessorImpl::SecondingAllowed
  ParachainProcessorImpl::secondingSanityCheck(
      const HypotheticalCandidate &hypothetical_candidate,
      bool backed_in_path_only) {
    const auto &active_leaves = our_current_state_.per_leaf;
    const auto &implicit_view = our_current_state_.implicit_view;

    fragment::FragmentTreeMembership membership;
    const auto &candidate_para = candidatePara(hypothetical_candidate);
    const auto &candidate_relay_parent = relayParent(hypothetical_candidate);
    const auto &candidate_hash = candidateHash(hypothetical_candidate);

    auto proc_response = [&](std::vector<size_t> &&depths,
                             const Hash &head,
                             const ActiveLeafState &leaf_state) {
      for (auto depth : depths) {
        if (auto it = leaf_state.seconded_at_depth.find(candidate_para);
            it != leaf_state.seconded_at_depth.end()
            && it->second.count(depth) != 0ull) {
          return false;
        }
      }
      membership.emplace_back(head, std::move(depths));
      return true;
    };

    for (const auto &[head, leaf_state] : active_leaves) {
      if (leaf_state.prospective_parachains_mode) {
        const auto allowed_parents_for_para =
            implicit_view.knownAllowedRelayParentsUnder(head, {candidate_para});
        if (std::find(allowed_parents_for_para.begin(),
                      allowed_parents_for_para.end(),
                      candidate_relay_parent)
            == allowed_parents_for_para.end()) {
          continue;
        }

        std::vector<size_t> r;
        for (auto &&[candidate, memberships] :
             prospective_parachains_->answerHypotheticalFrontierRequest(
                 gsl::span<const HypotheticalCandidate>{&hypothetical_candidate,
                                                        1},
                 {{head}},
                 backed_in_path_only)) {
          BOOST_ASSERT(candidateHash(candidate) == candidate_hash);
          for (auto &&[relay_parent, depths] : memberships) {
            BOOST_ASSERT(relay_parent == head);
            r.insert(r.end(), depths.begin(), depths.end());
          }
        }

        if (!proc_response(std::move(r), head, leaf_state)) {
          return std::nullopt;
        }
      } else {
        if (head == candidate_relay_parent) {
          if (auto it = leaf_state.seconded_at_depth.find(candidate_para);
              it != leaf_state.seconded_at_depth.end()
              && it->second.count(0) != 0ull) {
            return std::nullopt;
          }
          if (!proc_response(std::vector<size_t>{0ull}, head, leaf_state)) {
            return std::nullopt;
          }
        }
      }
    }

    if (membership.empty()) {
      return std::nullopt;
    }

    return membership;
  }

  bool ParachainProcessorImpl::canSecond(
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      ParachainId candidate_para_id,
      const Hash &relay_parent,
      const CandidateHash &candidate_hash,
      const Hash &parent_head_data_hash) {
    if (per_relay_parent.prospective_parachains_mode) {
      if (auto seconding_allowed = secondingSanityCheck(
              HypotheticalCandidateIncomplete{
                  .candidate_hash = candidate_hash,
                  .candidate_para = candidate_para_id,
                  .parent_head_data_hash = parent_head_data_hash,
                  .candidate_relay_parent = relay_parent},
              true)) {
        for (const auto &[_, m] : *seconding_allowed) {
          if (!m.empty()) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void ParachainProcessorImpl::handleAdvertisement(
      network::CollationEvent &&pending_collation,
      std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate) {
    REINVOKE(*this_context_,
             handleAdvertisement,
             std::move(pending_collation),
             std::move(prospective_candidate));

    const RelayHash &relay_parent =
        pending_collation.pending_collation.relay_parent;
    const libp2p::peer::PeerId &peer_id =
        pending_collation.pending_collation.peer_id;
    const ParachainId &para_id = pending_collation.pending_collation.para_id;

    auto opt_per_relay_parent = tryGetStateByRelayParent(relay_parent);
    if (!opt_per_relay_parent) {
      SL_TRACE(
          logger_, "Relay parent unknown. (relay_parent={})", relay_parent);
      return;
    }

    auto &per_relay_parent = opt_per_relay_parent->get();
    const ProspectiveParachainsModeOpt &relay_parent_mode =
        per_relay_parent.prospective_parachains_mode;
    const std::optional<network::ParachainId> &assignment =
        per_relay_parent.assignment;

    auto peer_state = pm_->getPeerState(peer_id);
    if (!peer_state) {
      SL_TRACE(logger_, "Unknown peer. (peerd_id={})", peer_id);
      return;
    }

    const auto collator_state = peer_state->get().collator_state;
    if (!collator_state) {
      SL_TRACE(logger_, "Undeclared collator. (peerd_id={})", peer_id);
      return;
    }

    const ParachainId collator_para_id = collator_state->para_id;
    if (!assignment) {
      SL_TRACE(logger_,
               "Invalid assignment. (peerd_id={}, collator={})",
               peer_id,
               collator_para_id);
      return;
    }

    if (relay_parent_mode && !prospective_candidate) {
      SL_WARN(logger_, "Protocol mismatch. (peer_id={})", peer_id);
      return;
    }

    const auto candidate_hash =
        utils::map(prospective_candidate,
                   [](const auto &pair) { return std::cref(pair.first); });

    auto insert_res = insertAdvertisement(
        peer_state->get(), relay_parent, relay_parent_mode, candidate_hash);
    if (insert_res.has_error()) {
      SL_TRACE(logger_,
               "Insert advertisement error. (error={})",
               insert_res.error().message());
      return;
    }

    const auto &[collator_id, parachain_id] = insert_res.value();
    if (!per_relay_parent.collations.hasSecondedSpace(relay_parent_mode)) {
      SL_TRACE(logger_, "Seconded limit reached.");
      return;
    }

    if (prospective_candidate) {
      auto &&[ch, parent_head_data_hash] = *prospective_candidate;
      const bool is_seconding_allowed = !relay_parent_mode
                                     || canSecond(per_relay_parent,
                                                  collator_para_id,
                                                  relay_parent,
                                                  ch,
                                                  parent_head_data_hash);

      if (!is_seconding_allowed) {
        SL_TRACE(logger_,
                 "Seconding is not allowed by backing, queueing advertisement. "
                 "(candidate hash={}, relay_parent = {}, para id={})",
                 ch,
                 relay_parent,
                 para_id);

        our_current_state_
            .blocked_advertisements[collator_para_id][parent_head_data_hash]
            .emplace_back(
                BlockedAdvertisement{.peer_id = peer_id,
                                     .collator_id = collator_id,
                                     .candidate_relay_parent = relay_parent,
                                     .candidate_hash = ch});

        return;
      }
    }

    if (auto result = enqueueCollation(per_relay_parent,
                                       relay_parent,
                                       para_id,
                                       peer_id,
                                       collator_id,
                                       std::move(prospective_candidate));
        result.has_error()) {
      SL_TRACE(logger_,
               "Failed to request advertised collation. (relay parent={}, para "
               "id={}, peer_id={}, error={})",
               relay_parent,
               para_id,
               peer_id,
               result.error().message());
    }
  }

  outcome::result<void> ParachainProcessorImpl::enqueueCollation(
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      const RelayHash &relay_parent,
      ParachainId para_id,
      const libp2p::peer::PeerId &peer_id,
      const CollatorId &collator_id,
      std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate) {
    BOOST_ASSERT(
        this_context_->io_context()->get_executor().running_in_this_thread());

    SL_TRACE(logger_,
             "Received advertise collation. (peer id={}, para id={}, relay "
             "parent={})",
             peer_id,
             para_id,
             relay_parent);

    const auto &relay_parent_mode =
        per_relay_parent.prospective_parachains_mode;
    auto &collations = per_relay_parent.collations;

    if (!collations.hasSecondedSpace(relay_parent_mode)) {
      SL_TRACE(logger_,
               "Limit of seconded collations reached for valid advertisement. "
               "(peer={}, para id={}, relay parent={})",
               peer_id,
               para_id,
               relay_parent);
      return outcome::success();
    }

    PendingCollation pending_collation{
        .relay_parent = relay_parent,
        .para_id = para_id,
        .peer_id = peer_id,
        .prospective_candidate = std::move(prospective_candidate)};

    switch (collations.status) {
      case CollationStatus::Fetching:
      case CollationStatus::WaitingOnValidation: {
        SL_TRACE(logger_,
                 "Added collation to the pending list. (peer_id={}, para "
                 "id={}, relay parent={})",
                 peer_id,
                 para_id,
                 relay_parent);

        collations.waiting_queue.emplace_back(std::move(pending_collation),
                                              collator_id);
      } break;
      case CollationStatus::Waiting: {
        fetchCollation(
            per_relay_parent, std::move(pending_collation), collator_id);
      } break;
      case CollationStatus::Seconded: {
        if (relay_parent_mode) {
          // Limit is not reached, it's allowed to second another
          // collation.
          fetchCollation(
              per_relay_parent, std::move(pending_collation), collator_id);
        } else {
          SL_TRACE(logger_,
                   "A collation has already been seconded. (peer_id={}, para "
                   "id={}, relay parent={})",
                   peer_id,
                   para_id,
                   relay_parent);
        }
      } break;
    }

    return outcome::success();
  }

  void ParachainProcessorImpl::fetchCollation(
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      PendingCollation &&pc,
      const CollatorId &id) {
    auto peer_state = pm_->getPeerState(pc.peer_id);
    if (!peer_state) {
      SL_TRACE(
          logger_, "No peer state. Unknown peer. (peer id={})", pc.peer_id);
      return;
    }

    const auto candidate_hash =
        utils::map(pc.prospective_candidate,
                   [](const auto &pair) { return std::cref(pair.first); });

    if (peer_state->get().hasAdvertised(pc.relay_parent, candidate_hash)) {
      fetchCollation(
          per_relay_parent, std::move(pc), id, peer_state->get().version);
      return;
    }
    SL_WARN(logger_, "Not advertised. (peer id={})", pc.peer_id);
  }

  void ParachainProcessorImpl::fetchCollation(
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      PendingCollation &&pc,
      const CollatorId &id,
      network::CollationVersion version) {
    if (our_current_state_.collation_requests_cancel_handles.count(pc)
        != 0ull) {
      SL_WARN(logger_,
              "Already requested. (relay parent={}, para id={})",
              pc.relay_parent,
              pc.para_id);
      return;
    }

    const auto peer_id = pc.peer_id;
    auto response_callback =
        [pending_collation{pc}, wptr{weak_from_this()}](
            outcome::result<network::CollationFetchingResponse>
                &&result) mutable {
          auto self = wptr.lock();
          if (!self) {
            return;
          }

          const RelayHash &relay_parent = pending_collation.relay_parent;
          const libp2p::peer::PeerId &peer_id = pending_collation.peer_id;

          SL_TRACE(self->logger_,
                   "Fetching collation from(peer={}, relay parent={})",
                   peer_id,
                   relay_parent);
          if (!result) {
            SL_WARN(self->logger_,
                    "Fetch collation from {}:{} failed with: {}",
                    peer_id,
                    relay_parent,
                    result.error());
            /// TODO(iceseer): do
            /// dequeue_next_collation_and_fetch
            return;
          }

          self->handleFetchedCollation(std::move(pending_collation),
                                       std::move(result).value());
        };

    network::CollationFetchingRequest fetch_collation_request{
        .relay_parent = pc.relay_parent, .para_id = pc.para_id};
    our_current_state_.collation_requests_cancel_handles.insert(std::move(pc));

    SL_TRACE(logger_,
             "Requesting collation. (peer id={}, para id={}, relay parent={})",
             pc.peer_id,
             pc.para_id,
             pc.relay_parent);

    const auto maybe_candidate_hash =
        utils::map(pc.prospective_candidate,
                   [](const auto &pair) { return std::cref(pair.first); });
    per_relay_parent.collations.status = CollationStatus::Fetching;
    per_relay_parent.collations.fetching_from.emplace(id, maybe_candidate_hash);

    router_->getReqCollationProtocol()->request(
        peer_id,
        std::move(fetch_collation_request),
        std::move(response_callback));
  }

}  // namespace kagome::parachain

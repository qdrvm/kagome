/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/parachain_processor.hpp"

#include <array>
#include <span>
#include <unordered_map>

#include <fmt/std.h>
#include <libp2p/common/final_action.hpp>

#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "crypto/hasher.hpp"
#include "crypto/sr25519_provider.hpp"
#include "dispute_coordinator/impl/runtime_info.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/fetch_attested_candidate.hpp"
#include "network/impl/protocols/parachain_protocols.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/candidate_view.hpp"
#include "parachain/peer_relay_parent_knowledge.hpp"
#include "scale/scale.hpp"
#include "utils/async_sequence.hpp"
#include "utils/map.hpp"
#include "utils/pool_handler.hpp"
#include "utils/profiler.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain,
                            ParachainProcessorImpl::Error,
                            e) {
  using E = kagome::parachain::ParachainProcessorImpl::Error;
  switch (e) {
    case E::RESPONSE_ALREADY_RECEIVED:
      return "Response already present";
    case E::REJECTED_BY_PROSPECTIVE_PARACHAINS:
      return "Rejected by prospective parachains";
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
    case E::NOT_CONFIRMED:
      return "Candidate not confirmed";
    case E::NO_STATE:
      return "No parachain state";
    case E::NO_SESSION_INFO:
      return "No session info";
    case E::OUT_OF_BOUND:
      return "Index out of bound";
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
      common::MainThreadPool &main_thread_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<network::PeerView> peer_view,
      common::WorkerThreadPool &worker_thread_pool,
      std::shared_ptr<parachain::BitfieldSigner> bitfield_signer,
      std::shared_ptr<parachain::PvfPrecheck> pvf_precheck,
      std::shared_ptr<parachain::BitfieldStore> bitfield_store,
      std::shared_ptr<parachain::BackingStore> backing_store,
      std::shared_ptr<parachain::Pvf> pvf,
      std::shared_ptr<parachain::AvailabilityStore> av_store,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      std::shared_ptr<parachain::ValidatorSignerFactory> signer_factory,
      const application::AppConfiguration &app_config,
      application::AppStateManager &app_state_manager,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      primitives::events::BabeStateSubscriptionEnginePtr babe_status_observable,
      std::shared_ptr<authority_discovery::Query> query_audi,
      std::shared_ptr<ProspectiveParachains> prospective_parachains)
      : pm_(std::move(pm)),
        runtime_info_(std::move(runtime_info)),
        crypto_provider_(std::move(crypto_provider)),
        router_(std::move(router)),
        main_pool_handler_{main_thread_pool.handler(app_state_manager)},
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
        chain_sub_{std::move(chain_sub_engine)},
        worker_pool_handler_{worker_thread_pool.handler(app_state_manager)},
        prospective_parachains_{std::move(prospective_parachains)} {
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(main_pool_handler_);
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
    BOOST_ASSERT(worker_pool_handler_);
    app_state_manager.takeControl(*this);

    our_current_state_.implicit_view.emplace(prospective_parachains_);
    BOOST_ASSERT(our_current_state_.implicit_view);

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

  void ParachainProcessorImpl::OnBroadcastBitfields(
      const primitives::BlockHash &relay_parent,
      const network::SignedBitfield &bitfield) {
    REINVOKE(*main_pool_handler_, OnBroadcastBitfields, relay_parent, bitfield);

    SL_TRACE(logger_, "Distribute bitfield on {}", relay_parent);
    auto relay_parent_state = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state) {
      SL_TRACE(logger_,
               "After `OnBroadcastBitfields` no parachain state on "
               "relay_parent. (relay "
               "parent={})",
               relay_parent);
      return;
    }

    send_to_validators_group(
        relay_parent,
        {network::VersionedValidatorProtocolMessage{
            network::vstaging::ValidatorProtocolMessage{
                network::vstaging::BitfieldDistributionMessage{
                    network::vstaging::BitfieldDistribution{relay_parent,
                                                            bitfield}}}}});
  }

  bool ParachainProcessorImpl::prepare() {
    bitfield_signer_->setBroadcastCallback(
        [wptr_self{weak_from_this()}](const primitives::BlockHash &relay_parent,
                                      const network::SignedBitfield &bitfield) {
          if (auto self = wptr_self.lock()) {
            self->OnBroadcastBitfields(relay_parent, bitfield);
          }
        });

    babe_status_observer_ =
        std::make_shared<primitives::events::BabeStateEventSubscriber>(
            babe_status_observable_, false);
    babe_status_observer_->setCallback(
        [wself{weak_from_this()}, was_synchronized = false](
            auto /*set_id*/,
            bool &synchronized,
            auto /*event_type*/,
            const primitives::events::SyncStateEventParams &event) mutable {
          if (auto self = wself.lock()) {
            if (event == consensus::SyncState::SYNCHRONIZED) {
              if (not was_synchronized) {
                self->bitfield_signer_->start();
                self->pvf_precheck_->start();
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
    babe_status_observer_->subscribe(
        babe_status_observer_->generateSubscriptionSetId(),
        primitives::events::SyncStateEventType::kSyncState);

    chain_sub_.onDeactivate(
        [wptr{weak_from_this()}](
            const primitives::events::RemoveAfterFinalizationParams &event) {
          if (auto self = wptr.lock()) {
            self->onDeactivateBlocks(event);
          }
        });

    my_view_sub_ = std::make_shared<network::PeerView::MyViewSubscriber>(
        peer_view_->getMyViewObservable(), false);
    primitives::events::subscribe(
        *my_view_sub_,
        network::PeerView::EventType::kViewUpdated,
        [wptr{weak_from_this()}](const network::ExView &event) {
          if (auto self = wptr.lock()) {
            self->onViewUpdated(event);
          }
        });

    remote_view_sub_ = std::make_shared<network::PeerView::PeerViewSubscriber>(
        peer_view_->getRemoteViewObservable(), false);
    primitives::events::subscribe(
        *remote_view_sub_,
        network::PeerView::EventType::kViewUpdated,
        [wptr{weak_from_this()}](const libp2p::peer::PeerId &peer_id,
                                 const network::View &view) {
          if (auto self = wptr.lock()) {
            self->onUpdatePeerView(peer_id, view);
          }
        });

    return true;
  }

  void ParachainProcessorImpl::onUpdatePeerView(
      const libp2p::peer::PeerId &peer_id, const network::View &view) {
    REINVOKE(*main_pool_handler_, onUpdatePeerView, peer_id, view);

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// `handle_peer_view_update` keep peer view to send only
    /// perfect messages
    for (const auto &h : view.heads_) {
      send_peer_messages_for_relay_parent({{peer_id}}, h);
    }
  }

  void ParachainProcessorImpl::send_peer_messages_for_relay_parent(
      std::optional<std::reference_wrapper<const libp2p::peer::PeerId>> peer_id,
      const RelayHash &relay_parent) {
    auto parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!parachain_state) {
      SL_WARN(logger_,
              "After `send_peer_messages_for_relay_parent` no parachain state "
              "on relay_parent. (relay_parent={})",
              relay_parent);
      return;
    }

    auto opt_session_info = retrieveSessionInfo(relay_parent);
    if (!opt_session_info) {
      SL_WARN(logger_, "No session info. (relay_parent={})", relay_parent);
      return;
    }

    Groups groups{opt_session_info->validator_groups};
    std::deque<network::VersionedValidatorProtocolMessage> messages;

    for (const auto &candidate_hash :
         parachain_state->get().issued_statements) {
      if (auto confirmed_candidate =
              candidates_.get_confirmed(candidate_hash)) {
        const auto group_index = confirmed_candidate->get().group_index();
        const auto group_size = groups.groups[group_index].size();

        auto local_knowledge =
            local_knowledge_filter(group_size,
                                   group_index,
                                   candidate_hash,
                                   *parachain_state->get().statement_store);
        network::VersionedValidatorProtocolMessage manifest{
            kagome::network::vstaging::ValidatorProtocolMessage{
                kagome::network::vstaging::StatementDistributionMessage{
                    kagome::network::vstaging::BackedCandidateManifest{
                        .relay_parent = relay_parent,
                        .candidate_hash = candidate_hash,
                        .group_index = group_index,
                        .para_id = confirmed_candidate->get().para_id(),
                        .parent_head_data_hash =
                            confirmed_candidate->get().parent_head_data_hash(),
                        .statement_knowledge = local_knowledge,
                    }}}};

        auto m = acknowledgement_and_statement_messages(
            *parachain_state->get().statement_store,
            groups.groups[group_index],
            local_knowledge,
            candidate_hash,
            relay_parent);

        messages.emplace_back(std::move(manifest));
        messages.insert(messages.end(),
                        std::move_iterator(m.begin()),
                        std::move_iterator(m.end()));
      }
    }

    if (peer_id) {
      auto se = pm_->getStreamEngine();
      BOOST_ASSERT(se);

      for (auto &msg : messages) {
        if (auto m =
                if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
          auto message = std::make_shared<network::WireMessage<
              network::vstaging::ValidatorProtocolMessage>>(
              std::move(m->get()));
          se->send(peer_id->get(),
                   router_->getValidationProtocolVStaging(),
                   message);
        }
      }
    } else {
      send_to_validators_group(relay_parent, std::move(messages));
    }
  }

  void ParachainProcessorImpl::onViewUpdated(const network::ExView &event) {
    REINVOKE(*main_pool_handler_, onViewUpdated, event);

    const auto &relay_parent = event.new_head.hash();
    if (auto r = canProcessParachains(); r.has_error()) {
      return;
    }

    if (const auto r =
            prospective_parachains_->onActiveLeavesUpdate(network::ExViewRef{
                .new_head = {event.new_head},
                .lost = event.lost,
            });
        r.has_error()) {
      SL_WARN(
          logger_,
          "Prospective parachains leaf update failed. (relay_parent={}, error={})",
          relay_parent,
          r.error().message());
    }

    backing_store_->onActivateLeaf(relay_parent);
    createBackingTask(relay_parent);
    SL_TRACE(logger_,
             "Update my view.(new head={}, finalized={}, leaves={})",
             relay_parent,
             event.view.finalized_number_,
             event.view.heads_.size());
    broadcastView(event.view);
    broadcastViewToGroup(relay_parent, event.view);
    for (const auto &h : event.view.heads_) {
      send_peer_messages_for_relay_parent(std::nullopt, h);
    }
    new_leaf_fragment_tree_updates(relay_parent);

    for (const auto &lost : event.lost) {
      SL_TRACE(logger_, "Removed backing task.(relay parent={})", lost);

      our_current_state_.per_leaf.erase(lost);
      our_current_state_.implicit_view->deactivate_leaf(lost);
      our_current_state_.state_by_relay_parent.erase(lost);
      pending_candidates.erase(lost);
      our_current_state_.active_leaves.erase(lost);
    }
    our_current_state_.active_leaves[relay_parent] =
        prospective_parachains_->prospectiveParachainsMode(relay_parent);

    for (auto it = our_current_state_.per_candidate.begin();
         it != our_current_state_.per_candidate.end();) {
      if (our_current_state_.state_by_relay_parent.find(it->second.relay_parent)
          != our_current_state_.state_by_relay_parent.end()) {
        ++it;
      } else {
        it = our_current_state_.per_candidate.erase(it);
      }
    }

    auto it_rp = our_current_state_.state_by_relay_parent.find(relay_parent);
    if (it_rp == our_current_state_.state_by_relay_parent.end()) {
      return;
    }

    std::vector<Hash> fresh_relay_parents;
    if (!it_rp->second.prospective_parachains_mode) {
      if (our_current_state_.per_leaf.find(relay_parent)
          != our_current_state_.per_leaf.end()) {
        return;
      }

      our_current_state_.per_leaf.emplace(
          relay_parent,
          ActiveLeafState{
              .prospective_parachains_mode = std::nullopt,
              .seconded_at_depth = {},
          });
      fresh_relay_parents.emplace_back(relay_parent);
    } else {
      auto frps =
          our_current_state_.implicit_view->knownAllowedRelayParentsUnder(
              relay_parent, std::nullopt);

      std::unordered_map<ParachainId, std::map<size_t, CandidateHash>>
          seconded_at_depth;
      for (const auto &[c_hash, cd] : our_current_state_.per_candidate) {
        if (!cd.seconded_locally) {
          continue;
        }

        fragment::FragmentTreeMembership membership =
            prospective_parachains_->answerTreeMembershipRequest(cd.para_id,
                                                                 c_hash);
        for (const auto &[h, depths] : membership) {
          if (h == relay_parent) {
            auto &mm = seconded_at_depth[cd.para_id];
            for (const auto depth : depths) {
              mm.emplace(depth, c_hash);
            }
          }
        }
      }

      our_current_state_.per_leaf.emplace(
          relay_parent,
          ActiveLeafState{
              .prospective_parachains_mode =
                  it_rp->second.prospective_parachains_mode,
              .seconded_at_depth = std::move(seconded_at_depth),
          });

      if (frps.empty()) {
        SL_WARN(logger_,
                "Implicit view gave no relay-parents. (leaf_hash={})",
                relay_parent);
        fresh_relay_parents.emplace_back(relay_parent);
      } else {
        fresh_relay_parents.insert(
            fresh_relay_parents.end(), frps.begin(), frps.end());
      }
    }

    for (const auto &maybe_new : fresh_relay_parents) {
      if (our_current_state_.state_by_relay_parent.find(maybe_new)
          != our_current_state_.state_by_relay_parent.end()) {
        continue;
      }
      createBackingTask(maybe_new);
    }
  }

  void ParachainProcessorImpl::onDeactivateBlocks(
      const primitives::events::RemoveAfterFinalizationParams &event) {
    REINVOKE(*main_pool_handler_, onDeactivateBlocks, event);

    for (const auto &lost : event) {
      SL_TRACE(logger_, "Remove from storages.(relay parent={})", lost);

      backing_store_->onDeactivateLeaf(lost);
      av_store_->remove(lost);
      bitfield_store_->remove(lost);
      our_current_state_.active_leaves.erase(lost);
    }
  }

  void ParachainProcessorImpl::broadcastViewExcept(
      const libp2p::peer::PeerId &peer_id, const network::View &view) const {
    auto msg = std::make_shared<
        network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
        network::ViewUpdate{.view = view});
    pm_->getStreamEngine()->broadcast(
        router_->getValidationProtocolVStaging(),
        msg,
        [&](const libp2p::peer::PeerId &p) { return peer_id != p; });
  }

  void ParachainProcessorImpl::broadcastViewToGroup(
      const primitives::BlockHash &relay_parent, const network::View &view) {
    auto opt_parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!opt_parachain_state) {
      SL_ERROR(
          logger_, "Relay state should exist. (relay_parent)", relay_parent);
      return;
    }

    std::deque<network::PeerId> group;
    if (auto r = runtime_info_->get_session_info(relay_parent)) {
      auto &[session, info] = r.value();
      if (info.our_group) {
        for (auto &i : session.validator_groups[*info.our_group]) {
          if (auto peer = query_audi_->get(session.discovery_keys[i])) {
            group.emplace_back(peer->id);
          }
        }
      }
    }

    auto protocol = [&]() -> std::shared_ptr<network::ProtocolBase> {
      return router_->getValidationProtocolVStaging();
    }();

    auto make_send = [&]<typename Msg>(
                         const Msg &msg,
                         const std::shared_ptr<network::ProtocolBase>
                             &protocol) {
      auto se = pm_->getStreamEngine();
      BOOST_ASSERT(se);

      auto message = std::make_shared<
          network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
          msg);
      SL_TRACE(
          logger_,
          "Broadcasting view update to group.(relay_parent={}, group_size={})",
          relay_parent,
          group.size());

      for (const auto &peer : group) {
        SL_TRACE(logger_, "Send to peer from group. (peer={})", peer);
        se->send(peer, protocol, message);
      }
    };

    make_send(network::vstaging::ViewUpdate{view},
              router_->getValidationProtocolVStaging());
  }

  void ParachainProcessorImpl::broadcastView(const network::View &view) const {
    auto msg = std::make_shared<
        network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
        network::ViewUpdate{.view = view});
    pm_->getStreamEngine()->broadcast(router_->getCollationProtocolVStaging(),
                                      msg);
    pm_->getStreamEngine()->broadcast(router_->getValidationProtocolVStaging(),
                                      msg);
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

  outcome::result<kagome::parachain::ParachainProcessorImpl::RelayParentState>
  ParachainProcessorImpl::initNewBackingTask(
      const primitives::BlockHash &relay_parent) {
    bool is_parachain_validator = false;
    ::libp2p::common::FinalAction metric_updater(
        [self{this}, &is_parachain_validator] {
          self->metric_is_parachain_validator_->set(is_parachain_validator);
        });
    OUTCOME_TRY(validators, parachain_host_->validators(relay_parent));
    OUTCOME_TRY(groups, parachain_host_->validator_groups(relay_parent));
    OUTCOME_TRY(cores, parachain_host_->availability_cores(relay_parent));
    OUTCOME_TRY(validator, isParachainValidator(relay_parent));
    OUTCOME_TRY(session_index,
                parachain_host_->session_index_for_child(relay_parent));
    OUTCOME_TRY(session_info,
                parachain_host_->session_info(relay_parent, session_index));
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
              std::get_if<network::ScheduledCore>(&cores[core_index])) {
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

    std::optional<StatementStore> statement_store;
    auto mode =
        prospective_parachains_->prospectiveParachainsMode(relay_parent);
    if (mode) {
      [[maybe_unused]] const auto _ =
          our_current_state_.implicit_view->activate_leaf(relay_parent);
      if (session_info) {
        std::unordered_map<GroupIndex, std::vector<ValidatorIndex>> groups;
        for (size_t g = 0; g < session_info->validator_groups.size(); ++g) {
          groups[g] = std::move(session_info->validator_groups[g]);
        }
        statement_store.emplace(Groups{std::move(groups)});
      }
    }

    SL_VERBOSE(logger_,
               "Inited new backing task v2.(assignment={}, our index={}, relay "
               "parent={})",
               assignment,
               validator->validatorIndex(),
               relay_parent);

    OUTCOME_TRY(
        minimum_backing_votes,
        parachain_host_->minimum_backing_votes(relay_parent, session_index));

    return RelayParentState{
        .prospective_parachains_mode = mode,
        .assignment = assignment,
        .seconded = {},
        .our_index = validator->validatorIndex(),
        .required_collator = required_collator,
        .collations = {},
        .table_context =
            TableContext{
                .validator = std::move(validator),
                .groups = std::move(out_groups),
                .validators = std::move(validators),
            },
        .statement_store = std::move(statement_store),
        .availability_cores = cores,
        .group_rotation_info = group_rotation_info,
        .minimum_backing_votes = minimum_backing_votes,
        .awaiting_validation = {},
        .issued_statements = {},
        .peers_advertised = {},
        .fallbacks = {},
    };
  }

  void ParachainProcessorImpl::createBackingTask(
      const primitives::BlockHash &relay_parent) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    auto rps_result = initNewBackingTask(relay_parent);
    if (rps_result.has_value()) {
      storeStateByRelayParent(relay_parent, std::move(rps_result.value()));
    } else if (rps_result.error() != Error::KEY_NOT_PRESENT) {
      logger_->error(
          "Relay parent state was not created. (relay parent={}, error={})",
          relay_parent,
          rps_result.error().message());
    }
  }

  void ParachainProcessorImpl::handleFetchedCollation(
      PendingCollation &&pending_collation,
      network::CollationFetchingResponse &&response) {
    REINVOKE(*main_pool_handler_,
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

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// fetched_candidates ???

    auto &parachain_state = opt_parachain_state->get();
    auto &assignment = parachain_state.assignment;
    // auto &seconded = parachain_state.seconded;
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

    pending_collation.commitments_hash = receipt.commitments_hash;

    std::optional<runtime::PersistedValidationData> pvd;
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

    if (!pvd) {
      SL_ERROR(
          logger_,
          "Persisted validation data not found. (relay parent={}, para={})",
          pending_collation.relay_parent,
          pending_collation.para_id);
      return;
    }

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// fetched_collation_sanity_check

    pending_candidates.insert(
        std::make_pair(pending_collation.relay_parent, pending_collation));

    collations.status = CollationStatus::WaitingOnValidation;
    validateAsync<ValidationTaskType::kSecond>(
        std::move(receipt),
        std::move(pov),
        std::move(*pvd),
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
  ParachainProcessorImpl::fetchPersistedValidationData(
      const RelayHash &relay_parent, ParachainId para_id) {
    return requestPersistedValidationData(relay_parent, para_id);
  }

  std::optional<runtime::PersistedValidationData>
  ParachainProcessorImpl::requestPersistedValidationData(
      const RelayHash &relay_parent, ParachainId para_id) {
    auto res_data = parachain_host_->persisted_validation_data(
        relay_parent, para_id, runtime::OccupiedCoreAssumption::Free);
    if (res_data.has_error()) {
      SL_VERBOSE(logger_,
                 "PersistedValidationData not found. (error={}, "
                 "relay_parent={} para_id={})",
                 res_data.error().message(),
                 relay_parent,
                 para_id);
      return std::nullopt;
    }
    return std::move(res_data.value());
  }

  void ParachainProcessorImpl::process_bitfield_distribution(
      const network::BitfieldDistributionMessage &val) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    auto bd{boost::get<const network::BitfieldDistribution>(&val)};
    BOOST_ASSERT_MSG(
        bd, "BitfieldDistribution is not present. Check message format.");

    auto opt_session_info = retrieveSessionInfo(bd->relay_parent);
    if (!opt_session_info) {
      SL_TRACE(logger_,
               "Unexpected relay parent. No session info. (validator index={}, "
               "relay_parent={})",
               bd->data.payload.ix,
               bd->relay_parent);
      return;
    }

    if (bd->data.payload.ix >= opt_session_info->validators.size()) {
      SL_TRACE(
          logger_,
          "Validator index out of bound. (validator index={}, relay_parent={})",
          bd->data.payload.ix,
          bd->relay_parent);
      return;
    }

    auto res_sc = SigningContext::make(parachain_host_, bd->relay_parent);
    if (res_sc.has_error()) {
      SL_TRACE(logger_,
               "Create signing context failed. (validator index={}, "
               "relay_parent={})",
               bd->data.payload.ix,
               bd->relay_parent);
      return;
    }
    SigningContext &context = res_sc.value();
    const auto buffer = context.signable(*hasher_, bd->data.payload.payload);

    auto res = crypto_provider_->verify(
        bd->data.signature,
        buffer,
        opt_session_info->validators[bd->data.payload.ix]);
    if (res.has_error() || !res.value()) {
      SL_TRACE(
          logger_,
          "Signature validation failed. (validator index={}, relay_parent={})",
          bd->data.payload.ix,
          bd->relay_parent);
      return;
    }

    SL_TRACE(logger_,
             "Imported bitfield {} {}",
             bd->data.payload.ix,
             bd->relay_parent);
    bitfield_store_->putBitfield(bd->relay_parent, bd->data);
  }

  ParachainProcessorImpl::ManifestImportSuccessOpt
  ParachainProcessorImpl::handle_incoming_manifest_common(
      const libp2p::peer::PeerId &peer_id,
      const CandidateHash &candidate_hash,
      const RelayHash &relay_parent,
      const ManifestSummary &manifest_summary,
      ParachainId para_id) {
    if (!candidates_.insert_unconfirmed(
            peer_id,
            candidate_hash,
            relay_parent,
            manifest_summary.claimed_group_index,
            {{manifest_summary.claimed_parent_hash, para_id}})) {
      SL_TRACE(logger_,
               "Insert unconfirmed candidate failed. (candidate hash={}, relay "
               "parent={}, para id={}, claimed parent={})",
               candidate_hash,
               relay_parent,
               para_id,
               manifest_summary.claimed_parent_hash);
      return std::nullopt;
    }

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// `grid_topology` and `local_validator`
    return ManifestImportSuccess{
        .acknowledge = false,
        .sender_index = 0,
    };
  }

  network::vstaging::StatementFilter
  ParachainProcessorImpl::local_knowledge_filter(
      size_t group_size,
      GroupIndex group_index,
      const CandidateHash &candidate_hash,
      const StatementStore &statement_store) {
    network::vstaging::StatementFilter f{group_size};
    statement_store.fill_statement_filter(group_index, candidate_hash, f);
    return f;
  }

  void ParachainProcessorImpl::send_to_validators_group(
      const RelayHash &relay_parent,
      const std::deque<network::VersionedValidatorProtocolMessage> &messages) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    auto relay_parent_state = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state) {
      SL_TRACE(logger_,
               "After `send_to_validators_group` no parachain state on "
               "relay_parent. (relay "
               "parent={})",
               relay_parent);
      return;
    }

    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

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
    for (const auto &p : group_set) {
      group.emplace_back(p);
    }

    auto protocol = [&]() -> std::shared_ptr<network::ProtocolBase> {
      return router_->getValidationProtocolVStaging();
    }();

    se->forEachPeer(protocol, [&](const network::PeerId &peer) {
      if (group_set.count(peer) == 0) {
        any.emplace_back(peer);
      }
    });
    auto lucky = kMinGossipPeers - std::min(group.size(), kMinGossipPeers);
    if (lucky != 0) {
      std::shuffle(any.begin(), any.end(), random_);
      any.erase(any.begin() + std::min(any.size(), lucky), any.end());
    } else {
      any.clear();
    }

    auto make_send = [&]<typename Msg>(
                         const Msg &msg,
                         const std::shared_ptr<network::ProtocolBase>
                             &protocol) {
      auto se = pm_->getStreamEngine();
      BOOST_ASSERT(se);

      auto message =
          std::make_shared<network::WireMessage<std::decay_t<decltype(msg)>>>(
              msg);
      logger_->trace(
          "Broadcasting messages.(relay_parent={}, group_size={}, "
          "lucky_size={})",
          relay_parent,
          group.size(),
          any.size());

      for (auto &peer : group) {
        SL_TRACE(logger_, "Send to peer from group. (peer={})", peer);
        se->send(peer, protocol, message);
      }

      for (auto &peer : any) {
        SL_TRACE(logger_, "Send to peer from any. (peer={})", peer);
        se->send(peer, protocol, message);
      }
    };

    for (const network::VersionedValidatorProtocolMessage &msg : messages) {
      visit_in_place(
          msg,
          [&](const kagome::network::vstaging::ValidatorProtocolMessage &m) {
            make_send(m, router_->getValidationProtocolVStaging());
          },
          [&](const kagome::network::ValidatorProtocolMessage &m) {
            make_send(m, router_->getValidationProtocol());
          });
    }
  }

  std::deque<network::VersionedValidatorProtocolMessage>
  ParachainProcessorImpl::acknowledgement_and_statement_messages(
      StatementStore &statement_store,
      const std::vector<ValidatorIndex> &group,
      const network::vstaging::StatementFilter &local_knowledge,
      const CandidateHash &candidate_hash,
      const RelayHash &relay_parent) {
    std::deque<network::VersionedValidatorProtocolMessage> messages;
    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// Will sent to the whole group. Optimize when `grid_view` will be
    /// implemented
    messages.emplace_back(network::VersionedValidatorProtocolMessage{
        network::vstaging::ValidatorProtocolMessage{
            network::vstaging::StatementDistributionMessage{
                network::vstaging::BackedCandidateAcknowledgement{
                    .candidate_hash = candidate_hash,
                    .statement_knowledge = local_knowledge,
                }}}});
    statement_store.groupStatements(
        group,
        candidate_hash,
        local_knowledge,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) {
          messages.emplace_back(network::VersionedValidatorProtocolMessage{
              network::vstaging::ValidatorProtocolMessage{
                  network::vstaging::StatementDistributionMessage{
                      network::vstaging::StatementDistributionMessageStatement{
                          .relay_parent = relay_parent,
                          .compact = statement,
                      }}}});
        });
    return messages;
  }

  std::deque<network::VersionedValidatorProtocolMessage>
  ParachainProcessorImpl::post_acknowledgement_statement_messages(
      const RelayHash &relay_parent,
      const StatementStore &statement_store,
      const std::vector<ValidatorIndex> &group,
      const CandidateHash &candidate_hash) {
    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// fill data from grid tracker
    network::vstaging::StatementFilter sending_filter{group.size()};

    std::deque<network::VersionedValidatorProtocolMessage> messages;
    statement_store.groupStatements(
        group,
        candidate_hash,
        sending_filter,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) {
          messages.emplace_back(network::VersionedValidatorProtocolMessage{
              network::vstaging::ValidatorProtocolMessage{
                  network::vstaging::StatementDistributionMessage{
                      network::vstaging::StatementDistributionMessageStatement{
                          .relay_parent = relay_parent,
                          .compact = statement,
                      }}}});
        });
    return messages;
  }

  void ParachainProcessorImpl::process_vstaging_statement(
      const libp2p::peer::PeerId &peer_id,
      const network::vstaging::StatementDistributionMessage &msg) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    SL_TRACE(
        logger_, "Incoming `StatementDistributionMessage`. (peer={})", peer_id);
    if (auto inner =
            if_type<const network::vstaging::BackedCandidateAcknowledgement>(
                msg)) {
      SL_TRACE(logger_,
               "`BackedCandidateAcknowledgement`. (candidate_hash={})",
               inner->get().candidate_hash);
      const network::vstaging::BackedCandidateAcknowledgement &acknowledgement =
          inner->get();
      const auto &candidate_hash = acknowledgement.candidate_hash;
      SL_TRACE(
          logger_,
          "Received incoming acknowledgement. (peer={}, candidate hash={})",
          peer_id,
          candidate_hash);

      auto c = candidates_.get_confirmed(candidate_hash);
      if (!c) {
        return;
      }
      const RelayHash &relay_parent = c->get().relay_parent();
      const Hash &parent_head_data_hash = c->get().parent_head_data_hash();
      GroupIndex group_index = c->get().group_index();
      ParachainId para_id = c->get().para_id();

      auto opt_parachain_state = tryGetStateByRelayParent(relay_parent);
      if (!opt_parachain_state) {
        SL_TRACE(
            logger_, "Handled statement from {} out of view", relay_parent);
        return;
      }
      auto &relay_parent_state = opt_parachain_state->get();
      BOOST_ASSERT(relay_parent_state.statement_store);

      std::optional<runtime::SessionInfo> opt_session_info =
          retrieveSessionInfo(relay_parent);
      if (!opt_session_info) {
        SL_WARN(logger_,
                "No session info for current parrent. (relay parent={})",
                relay_parent);
        return;
      }
      if (group_index >= opt_session_info->validator_groups.size()) {
        SL_WARN(logger_,
                "Group index out of bound. (relay parent={}, group={})",
                relay_parent,
                group_index);
        return;
      }
      const auto &group = opt_session_info->validator_groups[group_index];

      ManifestImportSuccessOpt x = handle_incoming_manifest_common(
          peer_id,
          candidate_hash,
          relay_parent,
          ManifestSummary{
              .claimed_parent_hash = parent_head_data_hash,
              .claimed_group_index = group_index,
              .statement_knowledge = acknowledgement.statement_knowledge,
          },
          para_id);
      if (!x) {
        return;
      }

      auto messages = post_acknowledgement_statement_messages(
          relay_parent,
          *relay_parent_state.statement_store,
          group,
          candidate_hash);
      if (!messages.empty()) {
        send_to_validators_group(relay_parent, messages);
      }
      return;
    }

    if (auto manifest =
            if_type<const network::vstaging::BackedCandidateManifest>(msg)) {
      SL_TRACE(logger_,
               "`BackedCandidateManifest`. (relay_parent={}, "
               "candidate_hash={}, para_id={}, parent_head_data_hash={})",
               manifest->get().relay_parent,
               manifest->get().candidate_hash,
               manifest->get().para_id,
               manifest->get().parent_head_data_hash);
      auto relay_parent_state =
          tryGetStateByRelayParent(manifest->get().relay_parent);
      if (!relay_parent_state) {
        SL_WARN(logger_,
                "After BackedCandidateManifest no parachain state on "
                "relay_parent. (relay "
                "parent={})",
                manifest->get().relay_parent);
        return;
      }

      if (!relay_parent_state->get().statement_store) {
        SL_ERROR(logger_,
                 "Statement store is not initialized. (relay parent={})",
                 manifest->get().relay_parent);
        return;
      }

      ManifestImportSuccessOpt x = handle_incoming_manifest_common(
          peer_id,
          manifest->get().candidate_hash,
          manifest->get().relay_parent,
          ManifestSummary{
              .claimed_parent_hash = manifest->get().parent_head_data_hash,
              .claimed_group_index = manifest->get().group_index,
              .statement_knowledge = manifest->get().statement_knowledge,
          },
          manifest->get().para_id);
      if (!x) {
        return;
      }

      std::optional<runtime::SessionInfo> opt_session_info =
          retrieveSessionInfo(manifest->get().relay_parent);
      if (!opt_session_info) {
        SL_WARN(logger_,
                "No session info for current parrent. (relay parent={})",
                manifest->get().relay_parent);
        return;
      }
      const auto &group =
          opt_session_info->validator_groups[manifest->get().group_index];

      if (x->acknowledge) {
        SL_TRACE(
            logger_,
            "Known candidate - acknowledging manifest. (candidate hash={})",
            manifest->get().candidate_hash);
        network::vstaging::StatementFilter local_knowledge =
            local_knowledge_filter(group.size(),
                                   manifest->get().group_index,
                                   manifest->get().candidate_hash,
                                   *relay_parent_state->get().statement_store);
        auto messages = acknowledgement_and_statement_messages(
            *relay_parent_state->get().statement_store,
            group,
            local_knowledge,
            manifest->get().candidate_hash,
            manifest->get().relay_parent);
        send_to_validators_group(manifest->get().relay_parent, messages);
      } else if (!candidates_.is_confirmed(manifest->get().candidate_hash)) {
        /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
        /// not used because of `acknowledge` = true. Implement `grid_view` to
        /// retrieve real `acknowledge`.

        network::vstaging::StatementFilter unwanted_mask{group.size()};
        router_->getFetchAttestedCandidateProtocol()->doRequest(
            peer_id,
            network::vstaging::AttestedCandidateRequest{
                .candidate_hash = manifest->get().candidate_hash,
                .mask = std::move(unwanted_mask),
            },
            [wptr{weak_from_this()},
             relay_parent{manifest->get().relay_parent},
             candidate_hash{manifest->get().candidate_hash},
             groups{Groups{opt_session_info->validator_groups}},
             group_index{manifest->get().group_index}](
                outcome::result<network::vstaging::AttestedCandidateResponse>
                    r) mutable {
              if (auto self = wptr.lock()) {
                self->handleFetchedStatementResponse(std::move(r),
                                                     relay_parent,
                                                     candidate_hash,
                                                     std::move(groups),
                                                     group_index);
              }
            });
      }
      return;
    }

    if (auto stm = if_type<
            const network::vstaging::StatementDistributionMessageStatement>(
            msg)) {
      SL_TRACE(logger_,
               "`StatementDistributionMessageStatement`. (relay_parent={}, "
               "candidate_hash={})",
               stm->get().relay_parent,
               candidateHash(getPayload(stm->get().compact)));
      auto parachain_state = tryGetStateByRelayParent(stm->get().relay_parent);
      if (!parachain_state) {
        SL_TRACE(logger_,
                 "After request pov no parachain state on relay_parent. (relay "
                 "parent={})",
                 stm->get().relay_parent);
        return;
      }

      auto opt_session_info = retrieveSessionInfo(stm->get().relay_parent);
      if (!opt_session_info) {
        SL_WARN(logger_,
                "No session info for current parrent. (relay parent={})",
                stm->get().relay_parent);
        return;
      }

      std::optional<GroupIndex> originator_group =
          [&]() -> std::optional<GroupIndex> {
        for (GroupIndex g = 0; g < opt_session_info->validator_groups.size();
             ++g) {
          const auto &group = opt_session_info->validator_groups[g];
          for (const auto &v : group) {
            if (v == stm->get().compact.payload.ix) {
              return g;
            }
          }
        }
        return std::nullopt;
      }();
      if (!originator_group) {
        SL_TRACE(logger_,
                 "No correct validator index in statement. (relay parent={}, "
                 "validator={})",
                 stm->get().relay_parent,
                 stm->get().compact.payload.ix);
        return;
      }

      const auto &candidate_hash =
          candidateHash(getPayload(stm->get().compact));
      const bool res = candidates_.insert_unconfirmed(peer_id,
                                                      candidate_hash,
                                                      stm->get().relay_parent,
                                                      *originator_group,
                                                      std::nullopt);
      if (!res) {
        return;
      }

      const auto confirmed = candidates_.get_confirmed(candidate_hash);
      const auto is_confirmed = candidates_.is_confirmed(candidate_hash);
      const auto &group = opt_session_info->validator_groups[*originator_group];

      if (!is_confirmed) {
        network::vstaging::StatementFilter unwanted_mask{group.size()};

        if (!parachain_state->get().statement_store) {
          SL_ERROR(logger_, "Statement store is not initialized.");
          return;
        }
        if (!parachain_state->get().prospective_parachains_mode) {
          SL_ERROR(logger_, "No prospective parachains.");
          return;
        }

        const auto seconding_limit =
            parachain_state->get()
                .prospective_parachains_mode->max_candidate_depth
            + 1;
        for (size_t i = 0; i < group.size(); ++i) {
          const auto &v = group[i];
          if (parachain_state->get().statement_store->seconded_count(v)
              >= seconding_limit) {
            unwanted_mask.seconded_in_group.bits[i] = true;
          }
        }

        router_->getFetchAttestedCandidateProtocol()->doRequest(
            peer_id,
            network::vstaging::AttestedCandidateRequest{
                .candidate_hash = candidate_hash,
                .mask = std::move(unwanted_mask),
            },
            [wptr{weak_from_this()},
             relay_parent{stm->get().relay_parent},
             candidate_hash,
             groups{Groups{opt_session_info->validator_groups}},
             group_index{*originator_group}](
                outcome::result<network::vstaging::AttestedCandidateResponse>
                    r) mutable {
              if (auto self = wptr.lock()) {
                self->handleFetchedStatementResponse(std::move(r),
                                                     relay_parent,
                                                     candidate_hash,
                                                     std::move(groups),
                                                     group_index);
              }
            });
      }

      /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
      /// check statement signature

      Groups groups{opt_session_info->validator_groups};
      const auto was_fresh_opt = parachain_state->get().statement_store->insert(
          groups, stm->get().compact, StatementOrigin::Remote);
      if (!was_fresh_opt) {
        SL_WARN(logger_,
                "Accepted message from unknown validator. (relay parent={}, "
                "validator={})",
                stm->get().relay_parent,
                stm->get().compact.payload.ix);
        return;
      }

      if (!*was_fresh_opt) {
        SL_TRACE(logger_,
                 "Statement was not fresh. (relay parent={}, validator={})",
                 stm->get().relay_parent,
                 stm->get().compact.payload.ix);
        return;
      }

      const auto is_importable = candidates_.is_importable(candidate_hash);
      if (is_importable && confirmed) {
        send_backing_fresh_statements(confirmed->get(),
                                      stm->get().relay_parent,
                                      parachain_state->get(),
                                      group,
                                      candidate_hash);
      }

      circulate_statement(stm->get().relay_parent, stm->get().compact);
      return;
    }

    SL_INFO(logger_, "Skipped message.");
  }

  void ParachainProcessorImpl::circulate_statement(
      const RelayHash &relay_parent,
      const IndexedAndSigned<network::vstaging::CompactStatement> &statement) {
    send_to_validators_group(
        relay_parent,
        {network::VersionedValidatorProtocolMessage{
            kagome::network::vstaging::ValidatorProtocolMessage{
                kagome::network::vstaging::StatementDistributionMessage{
                    kagome::network::vstaging::
                        StatementDistributionMessageStatement{
                            .relay_parent = relay_parent,
                            .compact = statement,
                        }}}}});
  }

  void ParachainProcessorImpl::handleFetchedStatementResponse(
      outcome::result<network::vstaging::AttestedCandidateResponse> &&r,
      const RelayHash &relay_parent,
      const CandidateHash &candidate_hash,
      Groups &&groups,
      GroupIndex group_index) {
    REINVOKE(*main_pool_handler_,
             handleFetchedStatementResponse,
             std::move(r),
             relay_parent,
             candidate_hash,
             std::move(groups),
             group_index);

    if (r.has_error()) {
      SL_INFO(logger_,
              "Fetch attested candidate returned an error. (relay parent={}, "
              "candidate={}, group index={}, error={})",
              relay_parent,
              candidate_hash,
              group_index,
              r.error());
      return;
    }

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// validate response

    auto parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!parachain_state) {
      SL_TRACE(
          logger_,
          "No relay parent data on fetch attested candidate response. (relay "
          "parent={})",
          relay_parent);
      return;
    }

    if (!parachain_state->get().statement_store) {
      SL_WARN(logger_,
              "No statement store. (relay parent={}, candidate={})",
              relay_parent,
              candidate_hash);
      return;
    }

    const network::vstaging::AttestedCandidateResponse &response = r.value();
    for (const auto &statement : response.statements) {
      parachain_state->get().statement_store->insert(
          groups, statement, StatementOrigin::Remote);
    }

    auto opt_post_confirmation =
        candidates_.confirm_candidate(candidate_hash,
                                      response.candidate_receipt,
                                      response.persisted_validation_data,
                                      group_index,
                                      hasher_);
    if (!opt_post_confirmation) {
      SL_WARN(logger_,
              "Candidate re-confirmed by request/response: logic error. (relay "
              "parent={}, candidate={})",
              relay_parent,
              candidate_hash);
      return;
    }

    auto &post_confirmation = *opt_post_confirmation;
    apply_post_confirmation(post_confirmation);

    auto opt_confirmed = candidates_.get_confirmed(candidate_hash);
    BOOST_ASSERT(opt_confirmed);

    if (!opt_confirmed->get().is_importable(std::nullopt)) {
      return;
    }

    auto it = groups.groups.find(group_index);
    if (it == groups.groups.end()) {
      SL_WARN(logger_,
              "Group was not found. (relay parent={}, candidate={}, group "
              "index={})",
              relay_parent,
              candidate_hash,
              group_index);
      return;
    }

    send_backing_fresh_statements(opt_confirmed->get(),
                                  relay_parent,
                                  parachain_state->get(),
                                  it->second,
                                  candidate_hash);
  }

  void ParachainProcessorImpl::new_confirmed_candidate_fragment_tree_updates(
      const HypotheticalCandidate &candidate) {
    fragment_tree_update_inner(std::nullopt, std::nullopt, {candidate});
  }

  void ParachainProcessorImpl::new_leaf_fragment_tree_updates(
      const Hash &leaf_hash) {
    fragment_tree_update_inner({leaf_hash}, std::nullopt, std::nullopt);
  }

  void
  ParachainProcessorImpl::prospective_backed_notification_fragment_tree_updates(
      ParachainId para_id, const Hash &para_head) {
    std::pair<std::reference_wrapper<const Hash>, ParachainId> p{{para_head},
                                                                 para_id};
    fragment_tree_update_inner(std::nullopt, p, std::nullopt);
  }

  void ParachainProcessorImpl::fragment_tree_update_inner(
      std::optional<std::reference_wrapper<const Hash>> active_leaf_hash,
      std::optional<std::pair<std::reference_wrapper<const Hash>, ParachainId>>
          required_parent_info,
      std::optional<std::reference_wrapper<const HypotheticalCandidate>>
          known_hypotheticals) {
    std::vector<HypotheticalCandidate> hypotheticals;
    if (!known_hypotheticals) {
      hypotheticals = candidates_.frontier_hypotheticals(required_parent_info);
    } else {
      hypotheticals.emplace_back(known_hypotheticals->get());
    }

    auto frontier = prospective_parachains_->answerHypotheticalFrontierRequest(
        hypotheticals, active_leaf_hash, false);
    for (const auto &[hypo, membership] : frontier) {
      if (membership.empty()) {
        continue;
      }

      for (const auto &[leaf_hash, _] : membership) {
        candidates_.note_importable_under(hypo, leaf_hash);
      }

      if (auto c = if_type<const HypotheticalCandidateComplete>(hypo)) {
        auto confirmed_candidate =
            candidates_.get_confirmed(c->get().candidate_hash);
        auto prs =
            tryGetStateByRelayParent(c->get().receipt.descriptor.relay_parent);

        if (prs && confirmed_candidate) {
          const auto group_index =
              group_for_para(prs->get().availability_cores,
                             prs->get().group_rotation_info,
                             c->get().receipt.descriptor.para_id);
          auto opt_session_info =
              retrieveSessionInfo(c->get().receipt.descriptor.relay_parent);
          if (!opt_session_info || !group_index
              || *group_index >= opt_session_info->validator_groups.size()) {
            return;
          }

          const auto &group = opt_session_info->validator_groups[*group_index];
          send_backing_fresh_statements(
              *confirmed_candidate,
              c->get().receipt.descriptor.relay_parent,
              prs->get(),
              group,
              c->get().candidate_hash);
        }
      }
    }
  }

  std::optional<GroupIndex> ParachainProcessorImpl::group_for_para(
      const std::vector<runtime::CoreState> &availability_cores,
      const runtime::GroupDescriptor &group_rotation_info,
      ParachainId para_id) const {
    std::optional<CoreIndex> core_index;
    for (CoreIndex i = 0; i < availability_cores.size(); ++i) {
      const auto c = visit_in_place(
          availability_cores[i],
          [](const runtime::OccupiedCore &core) -> std::optional<ParachainId> {
            return core.candidate_descriptor.para_id;
          },
          [](const runtime::ScheduledCore &core) -> std::optional<ParachainId> {
            return core.para_id;
          },
          [](const auto &) -> std::optional<ParachainId> {
            return std::nullopt;
          });

      if (c && *c == para_id) {
        core_index = i;
        break;
      }
    }

    if (!core_index) {
      return std::nullopt;
    }
    return group_rotation_info.groupForCore(*core_index,
                                            availability_cores.size());
  }

  void ParachainProcessorImpl::apply_post_confirmation(
      const PostConfirmation &post_confirmation) {
    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// `send_cluster_candidate_statements`

    new_confirmed_candidate_fragment_tree_updates(
        post_confirmation.hypothetical);
  }

  void ParachainProcessorImpl::send_backing_fresh_statements(
      const ConfirmedCandidate &confirmed,
      const RelayHash &relay_parent,
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      const std::vector<ValidatorIndex> &group,
      const CandidateHash &candidate_hash) {
    if (!per_relay_parent.statement_store) {
      return;
    }

    std::vector<std::pair<ValidatorIndex, network::vstaging::CompactStatement>>
        imported;
    per_relay_parent.statement_store->fresh_statements_for_backing(
        group,
        candidate_hash,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) {
          const auto &v = statement.payload.ix;
          const auto &compact = getPayload(statement);
          imported.emplace_back(v, compact);

          handleStatement(
              relay_parent,
              SignedFullStatementWithPVD{
                  .payload =
                      {
                          .payload = visit_in_place(
                              compact.inner_value,
                              [&](const network::vstaging::SecondedCandidateHash
                                      &) -> StatementWithPVD {
                                return StatementWithPVDSeconded{
                                    .committed_receipt = confirmed.receipt,
                                    .pvd = confirmed.persisted_validation_data,
                                };
                              },
                              [](const network::vstaging::ValidCandidateHash
                                     &val) -> StatementWithPVD {
                                return StatementWithPVDValid{
                                    .candidate_hash = val.hash,
                                };
                              },
                              [](const auto &) -> StatementWithPVD {
                                UNREACHABLE;
                              }),
                          .ix = statement.payload.ix,
                      },
                  .signature = statement.signature,
              });
        });

    for (const auto &[v, s] : imported) {
      per_relay_parent.statement_store->note_known_by_backing(v, s);
    }
  }

  void ParachainProcessorImpl::process_legacy_statement(
      const libp2p::peer::PeerId &peer_id,
      const network::StatementDistributionMessage &msg) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    if (auto statement_msg{boost::get<const network::Seconded>(&msg)}) {
      if (auto r = canProcessParachains(); r.has_error()) {
        return;
      }
      if (auto r = isParachainValidator(statement_msg->relay_parent);
          r.has_error() || !r.value()) {
        return;
      }

      SL_TRACE(
          logger_, "Imported statement on {}", statement_msg->relay_parent);

      std::optional<StatementWithPVD> stm;
      if (auto ccr = if_type<const network::CommittedCandidateReceipt>(
              getPayload(statement_msg->statement).candidate_state)) {
        std::optional<runtime::PersistedValidationData> pvd =
            fetchPersistedValidationData(statement_msg->relay_parent,
                                         ccr->get().descriptor.para_id);
        if (!pvd) {
          SL_TRACE(logger_, "No pvd fetched.");
          return;
        }
        stm = StatementWithPVDSeconded{
            .committed_receipt = ccr->get(),
            .pvd = std::move(*pvd),
        };
      } else if (auto h = if_type<const CandidateHash>(
                     getPayload(statement_msg->statement).candidate_state)) {
        stm = StatementWithPVDValid{
            .candidate_hash = h->get(),
        };
      }

      handleStatement(statement_msg->relay_parent,
                      SignedFullStatementWithPVD{
                          .payload =
                              {
                                  .payload = std::move(*stm),
                                  .ix = statement_msg->statement.payload.ix,
                              },
                          .signature = statement_msg->statement.signature,
                      });
    } else {
      const auto large = boost::get<const network::LargeStatement>(&msg);
      SL_ERROR(logger_,
               "Ignoring LargeStatement about {} from {}",
               large->payload.payload.candidate_hash,
               peer_id);
    }
  }

  void ParachainProcessorImpl::onValidationProtocolMsg(
      const libp2p::peer::PeerId &peer_id,
      const network::VersionedValidatorProtocolMessage &message) {
    REINVOKE(*main_pool_handler_, onValidationProtocolMsg, peer_id, message);

    SL_TRACE(
        logger_, "Incoming validator protocol message. (peer={})", peer_id);
    visit_in_place(
        message,
        [&](const network::ValidatorProtocolMessage &m) {
          SL_TRACE(logger_, "V1");
          visit_in_place(
              m,
              [&](const network::BitfieldDistributionMessage &val) {
                process_bitfield_distribution(val);
              },
              [&](const network::StatementDistributionMessage &val) {
                process_legacy_statement(peer_id, val);
              },
              [&](const auto &) {});
        },
        [&](const network::vstaging::ValidatorProtocolMessage &m) {
          SL_TRACE(logger_, "V2");
          visit_in_place(
              m,
              [&](const network::vstaging::BitfieldDistributionMessage &val) {
                process_bitfield_distribution(val);
              },
              [&](const network::vstaging::StatementDistributionMessage &val) {
                process_vstaging_statement(peer_id, val);
              },
              [&](const auto &) {});
        });

    //    if (auto
    //    msg{boost::get<network::StatementDistributionMessage>(&message)}) {
    //      return;
    //    }
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
      const runtime::PersistedValidationData &persisted_validation_data,
      RelayParentState &parachain_state) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    const auto candidate_hash{attesting_data.candidate.hash(*hasher_)};
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
      auto pvd{persisted_validation_data};
      requestPoV(
          *peer,
          candidate_hash,
          [candidate{attesting_data.candidate},
           pvd{std::move(pvd)},
           candidate_hash,
           wself{weak_from_this()},
           relay_parent,
           peer_id{peer->id}](auto &&pov_response_result) mutable {
            if (auto self = wself.lock()) {
              auto parachain_state =
                  self->tryGetStateByRelayParent(relay_parent);
              if (!parachain_state) {
                SL_TRACE(
                    self->logger_,
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
                  std::move(pvd),
                  peer_id,
                  relay_parent,
                  parachain_state->get().table_context.validators.size());
            }
          });
    }
  }

  outcome::result<network::vstaging::AttestedCandidateResponse>
  ParachainProcessorImpl::OnFetchAttestedCandidateRequest(
      const network::vstaging::AttestedCandidateRequest &request) {
    auto confirmed = candidates_.get_confirmed(request.candidate_hash);
    if (!confirmed) {
      return Error::NOT_CONFIRMED;
    }

    auto relay_parent_state =
        tryGetStateByRelayParent(confirmed->get().relay_parent());
    if (!relay_parent_state) {
      return Error::NO_STATE;
    }
    BOOST_ASSERT(relay_parent_state->get().statement_store);
    BOOST_ASSERT(relay_parent_state->get().our_index);

    std::optional<runtime::SessionInfo> opt_session_info =
        retrieveSessionInfo(confirmed->get().relay_parent());
    if (!opt_session_info) {
      return Error::NO_SESSION_INFO;
    }
    if (confirmed->get().group_index()
        >= opt_session_info->validator_groups.size()) {
      SL_ERROR(logger_,
               "Unexpected array bound for groups. (relay parent={})",
               confirmed->get().relay_parent());
      return Error::OUT_OF_BOUND;
    }
    const auto &group =
        opt_session_info->validator_groups[confirmed->get().group_index()];

    auto init_with_not = [](scale::BitVec &dst, const scale::BitVec &src) {
      dst.bits.reserve(src.bits.size());
      for (const auto i : src.bits) {
        dst.bits.emplace_back(!i);
      }
    };

    network::vstaging::StatementFilter and_mask;
    init_with_not(and_mask.seconded_in_group, request.mask.seconded_in_group);
    init_with_not(and_mask.validated_in_group, request.mask.validated_in_group);

    std::vector<IndexedAndSigned<network::vstaging::CompactStatement>>
        statements;
    relay_parent_state->get().statement_store->groupStatements(
        group,
        request.candidate_hash,
        and_mask,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) { statements.emplace_back(statement); });

    return network::vstaging::AttestedCandidateResponse{
        .candidate_receipt = confirmed->get().receipt,
        .persisted_validation_data = confirmed->get().persisted_validation_data,
        .statements = std::move(statements),
    };
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
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    const auto it = our_current_state_.state_by_relay_parent.find(relay_parent);
    if (it != our_current_state_.state_by_relay_parent.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  ParachainProcessorImpl::RelayParentState &
  ParachainProcessorImpl::storeStateByRelayParent(
      const primitives::BlockHash &relay_parent, RelayParentState &&val) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    const auto &[it, inserted] =
        our_current_state_.state_by_relay_parent.insert(
            {relay_parent, std::move(val)});
    BOOST_ASSERT(inserted);
    return it->second;
  }

  void ParachainProcessorImpl::handleStatement(
      const primitives::BlockHash &relay_parent,
      const SignedFullStatementWithPVD &statement) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    auto opt_parachain_state = tryGetStateByRelayParent(relay_parent);
    if (!opt_parachain_state) {
      logger_->trace("Handled statement from {} out of view", relay_parent);
      return;
    }

    auto &parachain_state = opt_parachain_state->get();
    auto &assignment = parachain_state.assignment;
    auto &fallbacks = parachain_state.fallbacks;
    auto &awaiting_validation = parachain_state.awaiting_validation;

    auto res = importStatement(relay_parent, statement, parachain_state);
    if (res.has_error()) {
      SL_TRACE(logger_,
               "Statement rejected. (relay_parent={}, error={}).",
               relay_parent,
               res.error());
      return;
    }

    post_import_statement_actions(relay_parent, parachain_state, res.value());
    if (auto result = res.value()) {
      if (result->group_id != assignment) {
        SL_TRACE(
            logger_,
            "Registered statement from not our group(our: {}, registered: {}).",
            assignment,
            result->group_id);
        return;
      }

      const auto &candidate_hash = result->candidate;
      SL_TRACE(logger_,
               "Registered incoming statement.(relay_parent={}).",
               relay_parent);
      std::optional<std::reference_wrapper<AttestingData>> attesting_ref =
          visit_in_place(
              parachain::getPayload(statement),
              [&](const StatementWithPVDSeconded &val)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto opt_candidate = backing_store_->getCadidateInfo(
                    relay_parent, candidate_hash);
                if (!opt_candidate) {
                  logger_->error("No candidate {}", candidate_hash);
                  return std::nullopt;
                }

                AttestingData attesting{
                    .candidate = candidateFromCommittedCandidateReceipt(
                        opt_candidate->get().candidate),
                    .pov_hash = val.committed_receipt.descriptor.pov_hash,
                    .from_validator = statement.payload.ix,
                    .backing = {}};

                auto const &[it, _] = fallbacks.insert(
                    std::make_pair(candidate_hash, std::move(attesting)));
                return it->second;
              },
              [&](const StatementWithPVDValid &val)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto it = fallbacks.find(val.candidate_hash);
                if (it == fallbacks.end()) {
                  return std::nullopt;
                }
                if (!parachain_state.our_index
                    || *parachain_state.our_index == statement.payload.ix) {
                  return std::nullopt;
                }
                if (awaiting_validation.find(val.candidate_hash)
                    != awaiting_validation.end()) {
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
        auto it = our_current_state_.per_candidate.find(candidate_hash);
        if (it != our_current_state_.per_candidate.end()) {
          kickOffValidationWork(relay_parent,
                                attesting_ref->get(),
                                it->second.persisted_validation_data,
                                parachain_state);
        }
      }
    }
  }

  std::optional<BackingStore::ImportResult>
  ParachainProcessorImpl::importStatementToTable(
      const RelayHash &relay_parent,
      ParachainProcessorImpl::RelayParentState &relayParentState,
      const primitives::BlockHash &candidate_hash,
      const network::SignedStatement &statement) {
    SL_TRACE(
        logger_, "Import statement into table.(candidate={})", candidate_hash);
    return backing_store_->put(
        relay_parent,
        relayParentState.table_context.groups,
        statement,
        relayParentState.prospective_parachains_mode.has_value());
  }

  void ParachainProcessorImpl::statementDistributionBackedCandidate(
      const CandidateHash &candidate_hash) {
    auto confirmed_opt = candidates_.get_confirmed(candidate_hash);
    if (!confirmed_opt) {
      SL_TRACE(logger_,
               "Received backed candidate notification for unknown or "
               "unconfirmed. (candidate_hash={})",
               candidate_hash);
      return;
    }
    const auto &confirmed = confirmed_opt->get();

    const auto relay_parent = confirmed.relay_parent();
    auto relay_parent_state_opt = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state_opt) {
      return;
    }
    BOOST_ASSERT(relay_parent_state_opt->get().statement_store);

    std::optional<runtime::SessionInfo> opt_session_info =
        retrieveSessionInfo(relay_parent);
    if (!opt_session_info) {
      return;
    }

    const auto group_index = confirmed.group_index();
    if (group_index >= opt_session_info->validator_groups.size()) {
      return;
    }
    const auto group_size =
        opt_session_info->validator_groups[group_index].size();

    /// `provide_candidate_to_grid`
    network::vstaging::StatementFilter filter =
        local_knowledge_filter(group_size,
                               group_index,
                               candidate_hash,
                               *relay_parent_state_opt->get().statement_store);

    std::deque<network::VersionedValidatorProtocolMessage> messages = {
        network::VersionedValidatorProtocolMessage{
            kagome::network::vstaging::ValidatorProtocolMessage{
                kagome::network::vstaging::StatementDistributionMessage{
                    kagome::network::vstaging::BackedCandidateManifest{
                        .relay_parent = relay_parent,
                        .candidate_hash = candidate_hash,
                        .group_index = group_index,
                        .para_id = confirmed.para_id(),
                        .parent_head_data_hash =
                            confirmed.parent_head_data_hash(),
                        .statement_knowledge = filter}}}},
        network::VersionedValidatorProtocolMessage{
            kagome::network::vstaging::ValidatorProtocolMessage{
                kagome::network::vstaging::StatementDistributionMessage{
                    kagome::network::vstaging::BackedCandidateAcknowledgement{
                        .candidate_hash = candidate_hash,
                        .statement_knowledge = filter}}}}};

    auto ex = post_acknowledgement_statement_messages(
        relay_parent,
        *relay_parent_state_opt->get().statement_store,
        opt_session_info->validator_groups[group_index],
        candidate_hash);
    messages.insert(messages.end(),
                    std::make_move_iterator(ex.begin()),
                    std::make_move_iterator(ex.end()));
    send_to_validators_group(relay_parent, messages);

    prospective_backed_notification_fragment_tree_updates(
        confirmed.para_id(), confirmed.para_head());
  }

  std::vector<network::BackedCandidate>
  ParachainProcessorImpl::getBackedCandidates(const RelayHash &relay_parent) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    auto relay_parent_state_opt = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state_opt) {
      return {};
    }

    if (relay_parent_state_opt->get().prospective_parachains_mode) {
      std::vector<network::BackedCandidate> backed;
      for (size_t core_idx = 0;
           core_idx < relay_parent_state_opt->get().availability_cores.size();
           ++core_idx) {
        const runtime::CoreState &core =
            relay_parent_state_opt->get().availability_cores[core_idx];
        std::optional<std::pair<CandidateHash, Hash>> response = visit_in_place(
            core,
            [&](const network::ScheduledCore &scheduled_core)
                -> std::optional<std::pair<CandidateHash, Hash>> {
              if (auto i = prospective_parachains_->answerGetBackableCandidates(
                      relay_parent, scheduled_core.para_id, 1, {});
                  !i.empty()) {
                return i[0];
              }
              return std::nullopt;
            },
            [&](const runtime::OccupiedCore &occupied_core)
                -> std::optional<std::pair<CandidateHash, Hash>> {
              /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
              /// `bitfields_indicate_availability` check
              if (occupied_core.next_up_on_available) {
                if (auto i =
                        prospective_parachains_->answerGetBackableCandidates(
                            relay_parent,
                            occupied_core.next_up_on_available->para_id,
                            1,
                            {occupied_core.candidate_hash});
                    !i.empty()) {
                  return i[0];
                }
                return std::nullopt;
              }
              return std::nullopt;
            },
            [&](const runtime::FreeCore &)
                -> std::optional<std::pair<CandidateHash, Hash>> {
              return std::nullopt;
            });

        if (!response) {
          SL_TRACE(logger_,
                   "No backable candidate returned by prospective parachains. "
                   "(relay_parent={}, core_idx={})",
                   relay_parent,
                   core_idx);
          continue;
        }

        const CandidateHash &c_hash = response->first;
        const RelayHash &r_hash = response->second;

        auto per_relay_state = tryGetStateByRelayParent(r_hash);
        if (!per_relay_state) {
          continue;
        }

        if (auto attested = attested_candidate(
                r_hash,
                c_hash,
                per_relay_state->get().table_context,
                per_relay_state->get().minimum_backing_votes)) {
          if (auto b = table_attested_to_backed(
                  std::move(*attested), per_relay_state->get().table_context)) {
            backed.emplace_back(std::move(*b));
          }
        }
      }
      return backed;
    } else {
      return backing_store_->get(relay_parent);
    }
  }

  std::optional<ParachainProcessorImpl::AttestedCandidate>
  ParachainProcessorImpl::attested(
      const network::CommittedCandidateReceipt &candidate,
      const BackingStore::StatementInfo &data,
      size_t validity_threshold) {
    const auto &validity_votes = data.validity_votes;
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
                ((ValidatorSignature &)val),
            };
          },
          [](const BackingStore::ValidityVoteValid &val) {
            return network::ValidityAttestation{
                network::ValidityAttestation::Explicit{},
                ((ValidatorSignature &)val),
            };
          });

      validity_votes_out.emplace_back(validator_index,
                                      std::move(validity_attestation));
    }

    return AttestedCandidate{
        .group_id = data.group_id,
        .candidate = candidate,
        .validity_votes = std::move(validity_votes_out),
    };
  }

  std::optional<ParachainProcessorImpl::AttestedCandidate>
  ParachainProcessorImpl::attested_candidate(
      const RelayHash &relay_parent,
      const CandidateHash &digest,
      const ParachainProcessorImpl::TableContext &context,
      uint32_t minimum_backing_votes) {
    if (auto opt_validity_votes =
            backing_store_->getCadidateInfo(relay_parent, digest)) {
      auto &data = opt_validity_votes->get();

      size_t len = std::numeric_limits<size_t>::max();
      if (auto it = context.groups.find(data.group_id);
          it != context.groups.end()) {
        len = it->second.size();
      }

      const auto v_threshold = std::min(len, size_t(minimum_backing_votes));
      return attested(data.candidate, data, v_threshold);
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

  outcome::result<std::optional<BackingStore::ImportResult>>
  ParachainProcessorImpl::importStatement(
      const network::RelayHash &relay_parent,
      const SignedFullStatementWithPVD &statement,
      ParachainProcessorImpl::RelayParentState &rp_state) {
    const CandidateHash candidate_hash =
        candidateHashFrom(parachain::getPayload(statement));

    SL_TRACE(logger_,
             "Importing statement.(relay_parent={}, validator_index={}, "
             "candidate_hash={})",
             relay_parent,
             statement.payload.ix,
             candidate_hash);

    if (auto seconded = if_type<const StatementWithPVDSeconded>(
            parachain::getPayload(statement));
        seconded
        && our_current_state_.per_candidate.find(candidate_hash)
               == our_current_state_.per_candidate.end()) {
      auto &candidate = seconded->get().committed_receipt;
      if (rp_state.prospective_parachains_mode) {
        fragment::FragmentTreeMembership membership =
            prospective_parachains_->introduceCandidate(
                candidate.descriptor.para_id,
                candidate,
                crypto::Hashed<const runtime::PersistedValidationData &,
                               32,
                               crypto::Blake2b_StreamHasher<32>>{
                    seconded->get().pvd},
                candidate_hash);
        if (membership.empty()) {
          SL_TRACE(logger_, "`membership` is empty.");
          return Error::REJECTED_BY_PROSPECTIVE_PARACHAINS;
        }

        prospective_parachains_->candidateSeconded(candidate.descriptor.para_id,
                                                   candidate_hash);
      }
      our_current_state_.per_candidate.insert(
          {candidate_hash,
           PerCandidateState{
               .persisted_validation_data = seconded->get().pvd,
               .seconded_locally = false,
               .para_id = seconded->get().committed_receipt.descriptor.para_id,
               .relay_parent =
                   seconded->get().committed_receipt.descriptor.relay_parent,
           }});
    }

    network::SignedStatement stmnt{
        .payload =
            {
                .payload = visit_in_place(
                    parachain::getPayload(statement),
                    [&](const StatementWithPVDSeconded &val) {
                      return network::CandidateState{val.committed_receipt};
                    },
                    [&](const StatementWithPVDValid &val) {
                      return network::CandidateState{val.candidate_hash};
                    }),
                .ix = statement.payload.ix,
            },
        .signature = statement.signature,
    };
    return importStatementToTable(
        relay_parent, rp_state, candidate_hash, stmnt);
  }

  void ParachainProcessorImpl::unblockAdvertisements(
      ParachainProcessorImpl::RelayParentState &rp_state,
      ParachainId para_id,
      const Hash &para_head) {
    std::optional<std::vector<BlockedAdvertisement>> unblocked{};
    auto it = our_current_state_.blocked_advertisements.find(para_id);
    if (it != our_current_state_.blocked_advertisements.end()) {
      auto i = it->second.find(para_head);
      if (i != it->second.end()) {
        unblocked = std::move(i->second);
        it->second.erase(i);
      }
    }

    if (unblocked) {
      requestUnblockedCollations(
          rp_state, para_id, para_head, std::move(*unblocked));
    }
  }

  void ParachainProcessorImpl::requestUnblockedCollations(
      ParachainProcessorImpl::RelayParentState &rp_state,
      ParachainId para_id,
      const Hash &para_head,
      std::vector<BlockedAdvertisement> &&blocked_vec) {
    for (auto blocked = blocked_vec.begin(); blocked != blocked_vec.end();) {
      const auto is_seconding_allowed =
          canSecond(rp_state,
                    para_id,
                    blocked->candidate_relay_parent,
                    blocked->candidate_hash,
                    para_head);
      if (is_seconding_allowed) {
        auto result =
            enqueueCollation(rp_state,
                             blocked->candidate_relay_parent,
                             para_id,
                             blocked->peer_id,
                             blocked->collator_id,
                             std::make_optional(std::make_pair(
                                 blocked->candidate_hash, para_head)));
        if (result.has_error()) {
          SL_DEBUG(logger_,
                   "Enqueue collation failed.(candidate={}, para id={}, "
                   "relay_parent={}, para_head={}, peer_id={})",
                   blocked->candidate_hash,
                   para_id,
                   blocked->candidate_relay_parent,
                   para_head,
                   blocked->peer_id);
        }
        blocked = blocked_vec.erase(blocked);
      } else {
        ++blocked;
      }
    }
    if (!blocked_vec.empty()) {
      our_current_state_.blocked_advertisements[para_id][para_head] =
          std::move(blocked_vec);
    }
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  outcome::result<
      std::optional<ParachainProcessorImpl::SignedFullStatementWithPVD>>
  ParachainProcessorImpl::sign_import_and_distribute_statement(
      ParachainProcessorImpl::RelayParentState &rp_state,
      const ValidateAndSecondResult &validation_result) {
    if (auto statement =
            createAndSignStatement<kStatementType>(validation_result)) {
      const SignedFullStatementWithPVD stm = visit_in_place(
          getPayload(*statement).candidate_state,
          [&](const network::CommittedCandidateReceipt &receipt)
              -> SignedFullStatementWithPVD {
            return SignedFullStatementWithPVD{
                .payload =
                    {
                        .payload =
                            StatementWithPVDSeconded{
                                .committed_receipt = receipt,
                                .pvd = validation_result.pvd,
                            },
                        .ix = statement->payload.ix,
                    },
                .signature = statement->signature,
            };
          },
          [&](const network::CandidateHash &candidateHash)
              -> SignedFullStatementWithPVD {
            return SignedFullStatementWithPVD{
                .payload =
                    {
                        .payload =
                            StatementWithPVDValid{
                                .candidate_hash = candidateHash,
                            },
                        .ix = statement->payload.ix,
                    },
                .signature = statement->signature,
            };
          },
          [&](const auto &) -> SignedFullStatementWithPVD {
            return SignedFullStatementWithPVD{};
          });

      OUTCOME_TRY(
          summary,
          importStatement(validation_result.relay_parent, stm, rp_state));
      share_local_statement_vstaging(
          rp_state, validation_result.relay_parent, stm);

      post_import_statement_actions(
          validation_result.relay_parent, rp_state, summary);
      return stm;
    }
    return std::nullopt;
  }

  void ParachainProcessorImpl::post_import_statement_actions(
      const RelayHash &relay_parent,
      ParachainProcessorImpl::RelayParentState &rp_state,
      std::optional<BackingStore::ImportResult> &summary) {
    if (!summary) {
      return;
    }

    SL_TRACE(logger_,
             "Import result.(candidate={}, group id={}, validity votes={})",
             summary->candidate,
             summary->group_id,
             summary->validity_votes);

    if (auto attested = attested_candidate(relay_parent,
                                           summary->candidate,
                                           rp_state.table_context,
                                           rp_state.minimum_backing_votes)) {
      if (rp_state.backed_hashes
              .insert(candidateHash(*hasher_, attested->candidate))
              .second) {
        if (auto backed = table_attested_to_backed(std::move(*attested),
                                                   rp_state.table_context)) {
          const auto para_id = backed->candidate.descriptor.para_id;
          SL_INFO(
              logger_,
              "Candidate backed.(candidate={}, para id={}, relay_parent={})",
              summary->candidate,
              summary->group_id,
              relay_parent);
          if (rp_state.prospective_parachains_mode) {
            prospective_parachains_->candidateBacked(para_id,
                                                     summary->candidate);
            unblockAdvertisements(
                rp_state, para_id, backed->candidate.descriptor.para_head_hash);
            statementDistributionBackedCandidate(summary->candidate);
          } else {
            backing_store_->add(relay_parent, std::move(*backed));
          }
        }
      }
    }
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  std::optional<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatement(
      const ValidateAndSecondResult &validation_result) {
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
              network::CandidateState{network::CommittedCandidateReceipt{
                  .descriptor = validation_result.candidate.descriptor,
                  .commitments = *validation_result.commitments}}},
          *parachain_state->get().our_index,
          parachain_state->get());
    } else if constexpr (kStatementType == StatementType::kValid) {
      return createAndSignStatementFromPayload(
          network::Statement{network::CandidateState{
              validation_result.candidate.hash(*hasher_)}},
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
              self->logger_->verbose("Unable to create stream {} with {}: {}",
                                     protocol->protocolName(),
                                     peer_id,
                                     stream_result.error());
              return;
            }

            auto stream = stream_result.value();
            stream_engine->addOutgoing(std::move(stream_result.value()),
                                       protocol);

            std::forward<F>(callback)(std::move(stream));
          });
      return true;
    }
    return false;
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingCollatingStream(
      const libp2p::peer::PeerId &peer_id, F &&callback) {
    auto protocol = router_->getCollationProtocolVStaging();
    BOOST_ASSERT(protocol);

    return tryOpenOutgoingStream(
        peer_id, std::move(protocol), std::forward<F>(callback));
  }

  template <typename F>
  bool ParachainProcessorImpl::tryOpenOutgoingValidationStream(
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      F &&callback) {
    std::shared_ptr<network::ProtocolBase> protocol;
    switch (version) {
      case network::CollationVersion::V1:
      case network::CollationVersion::VStaging: {
        protocol = router_->getValidationProtocolVStaging();
      } break;
      default: {
        UNREACHABLE;
      } break;
    }
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
            network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
            network::ViewUpdate{.view = my_view->get().view}));
  }

  void ParachainProcessorImpl::onIncomingCollationStream(
      const libp2p::peer::PeerId &peer_id, network::CollationVersion version) {
    REINVOKE(*main_pool_handler_, onIncomingCollationStream, peer_id, version);

    auto peer_state = [&]() {
      auto res = pm_->getPeerState(peer_id);
      if (!res) {
        SL_TRACE(logger_, "From unknown peer {}", peer_id);
        res = pm_->createDefaultPeerState(peer_id);
      }
      return res;
    }();

    peer_state->get().version = version;
    if (tryOpenOutgoingCollatingStream(
            peer_id, [wptr{weak_from_this()}, peer_id, version](auto &&stream) {
              if (auto self = wptr.lock()) {
                switch (version) {
                  case network::CollationVersion::V1:
                  case network::CollationVersion::VStaging: {
                    self->sendMyView(
                        peer_id,
                        stream,
                        self->router_->getCollationProtocolVStaging());
                  } break;
                  default: {
                    UNREACHABLE;
                  } break;
                }
              }
            })) {
      SL_DEBUG(logger_, "Initiated collation protocol with {}", peer_id);
    }
  }

  void ParachainProcessorImpl::onIncomingValidationStream(
      const libp2p::peer::PeerId &peer_id, network::CollationVersion version) {
    REINVOKE(*main_pool_handler_, onIncomingValidationStream, peer_id, version);

    SL_TRACE(logger_, "Received incoming validation stream {}", peer_id);
    auto peer_state = [&]() {
      auto res = pm_->getPeerState(peer_id);
      if (!res) {
        SL_TRACE(logger_, "From unknown peer {}", peer_id);
        res = pm_->createDefaultPeerState(peer_id);
      }
      return res;
    }();

    peer_state->get().version = version;
    if (tryOpenOutgoingValidationStream(
            peer_id,
            version,
            [wptr{weak_from_this()}, peer_id, version](auto &&stream) {
              if (auto self = wptr.lock()) {
                switch (version) {
                  case network::CollationVersion::V1:
                  case network::CollationVersion::VStaging: {
                    self->sendMyView(
                        peer_id,
                        stream,
                        self->router_->getValidationProtocolVStaging());
                  } break;
                  default: {
                    UNREACHABLE;
                  } break;
                }
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

    auto collation_protocol = router_->getCollationProtocolVStaging();
    BOOST_ASSERT(collation_protocol);

    auto &statements_queue = our_current_state_.seconded_statements[peer_id];
    while (!statements_queue.empty()) {
      auto p{std::move(statements_queue.front())};
      statements_queue.pop_front();
      RelayHash &rp = p.first;

      network::SignedStatement statement = visit_in_place(
          getPayload(p.second),
          [&](const StatementWithPVDSeconded &s) -> network::SignedStatement {
            return {
                .payload =
                    {
                        .payload = network::CandidateState{s.committed_receipt},
                        .ix = p.second.payload.ix,
                    },
                .signature = p.second.signature,
            };
          },
          [&](const StatementWithPVDValid &s) -> network::SignedStatement {
            return {
                .payload =
                    {
                        .payload = network::CandidateState{s.candidate_hash},
                        .ix = p.second.payload.ix,
                    },
                .signature = p.second.signature,
            };
          });

      pending_candidates.erase(rp);
      stream_engine->send(
          peer_id,
          collation_protocol,
          std::make_shared<
              network::WireMessage<network::vstaging::CollatorProtocolMessage>>(
              network::vstaging::CollatorProtocolMessage(
                  network::vstaging::CollationMessage(
                      network::vstaging::
                          CollatorProtocolMessageCollationSeconded{
                              .relay_parent = rp,
                              .statement = std::move(statement)}))));
    }
  }

  void ParachainProcessorImpl::notify(
      const libp2p::peer::PeerId &peer_id,
      const primitives::BlockHash &relay_parent,
      const SignedFullStatementWithPVD &statement) {
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
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
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
      const ValidateAndSecondResult &validation_result) {
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
      SL_WARN(logger_,
              "Candidate {} validation failed with: {}",
              candidate_hash,
              validation_result.result.error());
      /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
      /// send invalid
      return;
    }

    if (!seconded
        && parachain_state.issued_statements.count(candidate_hash) == 0) {
      logger_->trace(
          "Second candidate complete. (candidate={}, peer={}, relay parent={})",
          candidate_hash,
          peer_id,
          validation_result.relay_parent);

      const auto parent_head_data_hash =
          hasher_->blake2b_256(validation_result.pvd.parent_head);
      const auto ph =
          hasher_->blake2b_256(validation_result.commitments->para_head);
      if (parent_head_data_hash == ph) {
        return;
      }

      HypotheticalCandidateComplete hypothetical_candidate{
          .candidate_hash = candidate_hash,
          .receipt =
              network::CommittedCandidateReceipt{
                  .descriptor = validation_result.candidate.descriptor,
                  .commitments = *validation_result.commitments,
              },
          .persisted_validation_data = validation_result.pvd,
      };

      fragment::FragmentTreeMembership fragment_tree_membership;
      if (auto seconding_allowed =
              secondingSanityCheck(hypothetical_candidate, false)) {
        fragment_tree_membership = std::move(*seconding_allowed);
      } else {
        return;
      }

      seconded = candidate_hash;

      auto res = sign_import_and_distribute_statement<StatementType::kSeconded>(
          parachain_state, validation_result);
      if (res.has_error()) {
        SL_WARN(logger_,
                "Attempted to second candidate but was rejected by prospective "
                "parachains. (candidate_hash={}, relay_parent={}, error={})",
                candidate_hash,
                validation_result.relay_parent,
                res.error());
        /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
        /// send invalid
        return;
      }

      if (!res.value()) {
        return;
      }

      auto &stmt = *res.value();
      if (auto it = our_current_state_.per_candidate.find(candidate_hash);
          it != our_current_state_.per_candidate.end()) {
        it->second.seconded_locally = true;
      } else {
        SL_WARN(logger_,
                "Missing `per_candidate` for seconded candidate. (candidate "
                "hash={})",
                candidate_hash);
      }

      for (const auto &[leaf, depths] : fragment_tree_membership) {
        auto it = our_current_state_.per_leaf.find(leaf);
        if (it == our_current_state_.per_leaf.end()) {
          SL_WARN(logger_,
                  "Missing `per_leaf` for known active leaf. (leaf={})",
                  leaf);
          continue;
        }

        ActiveLeafState &leaf_data = it->second;
        auto &seconded_at_depth =
            leaf_data.seconded_at_depth[validation_result.candidate.descriptor
                                            .para_id];

        for (const auto &depth : depths) {
          seconded_at_depth.emplace(depth, candidate_hash);
        }
      }

      parachain_state.issued_statements.insert(candidate_hash);
      notify(peer_id, validation_result.relay_parent, stmt);
    }
  }

  void ParachainProcessorImpl::share_local_statement_v1(
      RelayParentState &per_relay_parent,
      const primitives::BlockHash &relay_parent,
      const SignedFullStatementWithPVD &statement) {
    send_to_validators_group(
        relay_parent,
        {network::ValidatorProtocolMessage{
            network::StatementDistributionMessage{network::Seconded{
                .relay_parent = relay_parent,
                .statement = network::SignedStatement{
                    .payload =
                        {
                            .payload = visit_in_place(
                                parachain::getPayload(statement),
                                [&](const StatementWithPVDSeconded &val) {
                                  return network::CandidateState{
                                      val.committed_receipt};
                                },
                                [&](const StatementWithPVDValid &val) {
                                  return network::CandidateState{
                                      val.candidate_hash};
                                }),
                            .ix = statement.payload.ix,
                        },
                    .signature = statement.signature,
                }}}}});
  }

  void ParachainProcessorImpl::share_local_statement_vstaging(
      RelayParentState &per_relay_parent,
      const primitives::BlockHash &relay_parent,
      const SignedFullStatementWithPVD &statement) {
    const CandidateHash candidate_hash =
        candidateHashFrom(getPayload(statement));
    SL_TRACE(logger_,
             "Sharing statement. (relay parent={}, candidate hash={})",
             relay_parent,
             candidate_hash);

    BOOST_ASSERT(per_relay_parent.our_index);
    std::optional<runtime::SessionInfo> opt_session_info =
        retrieveSessionInfo(relay_parent);
    if (!opt_session_info) {
      SL_ERROR(logger_,
               "Retrieve session info failed. (relay parent={})",
               relay_parent);
      return;
    }

    Groups groups{opt_session_info->validator_groups};

    const std::optional<network::ParachainId> &local_assignment =
        per_relay_parent.assignment;
    const network::ValidatorIndex local_index = *per_relay_parent.our_index;
    const auto local_group_opt = groups.byValidatorIndex(local_index);
    if (!opt_session_info) {
      SL_ERROR(logger_,
               "Local validator info is not present. (relay parent={})",
               relay_parent);
      return;
    }
    const GroupIndex local_group = *local_group_opt;

    std::optional<std::pair<ParachainId, Hash>> expected = visit_in_place(
        getPayload(statement),
        [&](const StatementWithPVDSeconded &v)
            -> std::optional<std::pair<ParachainId, Hash>> {
          return std::make_pair(v.committed_receipt.descriptor.para_id,
                                v.committed_receipt.descriptor.relay_parent);
        },
        [&](const StatementWithPVDValid &v)
            -> std::optional<std::pair<ParachainId, Hash>> {
          if (auto p = candidates_.get_confirmed(v.candidate_hash)) {
            return std::make_pair(p->get().para_id(), p->get().relay_parent());
          }
          return std::nullopt;
        });
    const bool is_seconded =
        is_type<StatementWithPVDSeconded>(getPayload(statement));

    if (!expected) {
      SL_ERROR(
          logger_, "Invalid share statement. (relay parent={})", relay_parent);
      return;
    }
    const auto &[expected_para, expected_relay_parent] = *expected;

    if (local_index != statement.payload.ix) {
      SL_ERROR(logger_,
               "Invalid share statement because of validator index. (relay "
               "parent={})",
               relay_parent);
      return;
    }

    BOOST_ASSERT(per_relay_parent.statement_store);
    BOOST_ASSERT(per_relay_parent.prospective_parachains_mode);

    const auto seconding_limit =
        per_relay_parent.prospective_parachains_mode->max_candidate_depth + 1;
    if (is_seconded
        && per_relay_parent.statement_store->seconded_count(local_index)
               == seconding_limit) {
      SL_WARN(
          logger_,
          "Local node has issued too many `Seconded` statements. (limit={})",
          seconding_limit);
      return;
    }

    if (!local_assignment || *local_assignment != expected_para
        || relay_parent != expected_relay_parent) {
      SL_ERROR(
          logger_,
          "Invalid share statement because local assignment. (relay parent={})",
          relay_parent);
      return;
    }

    IndexedAndSigned<network::vstaging::CompactStatement> compact_statement =
        signed_to_compact(statement);
    std::optional<PostConfirmation> post_confirmation;
    if (auto s =
            if_type<const StatementWithPVDSeconded>(getPayload(statement))) {
      post_confirmation =
          candidates_.confirm_candidate(candidate_hash,
                                        s->get().committed_receipt,
                                        s->get().pvd,
                                        local_group,
                                        hasher_);
    }

    if (auto r = per_relay_parent.statement_store->insert(
            groups, compact_statement, StatementOrigin::Local);
        !r || !*r) {
      SL_ERROR(logger_,
               "Invalid share statement because statement store insertion "
               "failed. (relay parent={})",
               relay_parent);
      return;
    }

    circulate_statement(relay_parent, compact_statement);
    if (post_confirmation) {
      apply_post_confirmation(*post_confirmation);
    }
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
    REINVOKE(*main_pool_handler_,
             makeAvailable<kMode>,
             peer_id,
             candidate_hash,
             std::move(validate_and_second_result));

    auto parachain_state =
        tryGetStateByRelayParent(validate_and_second_result.relay_parent);
    if (!parachain_state) {
      SL_TRACE(logger_,
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
    REINVOKE(*main_pool_handler_,
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

    auto _measure = std::make_shared<TicToc>("Parachain validation", logger_);
    const auto candidate_hash{candidate.hash(*hasher_)};

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// checks if we still need to execute parachain task
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
    auto cb = [weak_self{weak_from_this()},
               candidate,
               pov,
               pvd,
               peer_id,
               relay_parent,
               n_validators,
               _measure,
               candidate_hash](
                  outcome::result<Pvf::Result> validation_result) mutable {
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      if (!validation_result) {
        SL_WARN(
            self->logger_,
            "Candidate {} on relay_parent {}, para_id {} validation failed with "
            "error: {}",
            candidate_hash,
            candidate.descriptor.relay_parent,
            candidate.descriptor.para_id,
            validation_result.error().message());
        return;
      }

      /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
      /// checks if we still need to execute parachain task
      auto need_to_process =
          self->our_current_state_.active_leaves.count(relay_parent) != 0ull;

      if (!need_to_process) {
        SL_TRACE(
            self->logger_,
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

      auto chunks_res =
          self->validateErasureCoding(available_data, n_validators);
      if (chunks_res.has_error()) {
        SL_WARN(self->logger_,
                "Erasure coding validation failed. (error={})",
                chunks_res.error());
        return;
      }
      auto &chunks = chunks_res.value();

      self->notifyAvailableData(std::move(chunks),
                                relay_parent,
                                candidate_hash,
                                available_data.pov,
                                available_data.validation_data);

      self->makeAvailable<kMode>(
          peer_id,
          candidate_hash,
          ValidateAndSecondResult{
              .result = outcome::success(),
              .relay_parent = relay_parent,
              .commitments = std::make_shared<network::CandidateCommitments>(
                  std::move(comms)),
              .candidate = candidate,
              .pov = std::move(available_data.pov),
              .pvd = std::move(pvd),
          });
    };
    pvf_->pvf(candidate,
              pov,
              pvd,
              [weak_self{weak_from_this()},
               cb{std::move(cb)}](outcome::result<Pvf::Result> r) mutable {
                auto self = weak_self.lock();
                if (not self) {
                  return;
                }
                post(*self->main_pool_handler_,
                     [cb{std::move(cb)}, r{std::move(r)}]() mutable {
                       cb(std::move(r));
                     });
              });
  }

  void ParachainProcessorImpl::onAttestComplete(
      const libp2p::peer::PeerId &, const ValidateAndSecondResult &result) {
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
        if (const auto r =
                sign_import_and_distribute_statement<StatementType::kValid>(
                    parachain_state->get(), result);
            r.has_error()) {
          SL_WARN(logger_,
                  "Sign import and distribute failed. (relay_parent={}, "
                  "candidate_hash={}, para_id={}, error={})",
                  result.relay_parent,
                  candidate_hash,
                  result.candidate.descriptor.para_id,
                  r.error());
          return;
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
      auto it = our_current_state_.per_candidate.find(candidate_hash);
      if (it != our_current_state_.per_candidate.end()) {
        kickOffValidationWork(relay_parent,
                              attesting,
                              it->second.persisted_validation_data,
                              *parachain_state);
      }
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
                                     *our_current_state_.implicit_view,
                                     our_current_state_.active_leaves,
                                     peer_data.collator_state->para_id)) {
      SL_TRACE(logger_, "Out of view. (relay_parent={})", on_relay_parent);
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
      if (candidates.size() > relay_parent_mode->max_candidate_depth) {
        return Error::PEER_LIMIT_REACHED;
      }
      auto [_, inserted] = candidates.insert(*candidate_hash);
      if (!inserted) {
        return Error::DUPLICATE;
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
    const auto &implicit_view = *our_current_state_.implicit_view;

    fragment::FragmentTreeMembership membership;
    const auto candidate_para = candidatePara(hypothetical_candidate);
    const auto candidate_relay_parent = relayParent(hypothetical_candidate);
    const auto candidate_hash = candidateHash(hypothetical_candidate);

    auto proc_response = [&](std::vector<size_t> &&depths,
                             const Hash &head,
                             const ActiveLeafState &leaf_state) {
      for (auto depth : depths) {
        if (auto it = leaf_state.seconded_at_depth.find(candidate_para.get());
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
            implicit_view.knownAllowedRelayParentsUnder(head,
                                                        {candidate_para.get()});
        if (std::find(allowed_parents_for_para.begin(),
                      allowed_parents_for_para.end(),
                      candidate_relay_parent.get())
            == allowed_parents_for_para.end()) {
          continue;
        }

        std::vector<size_t> r;
        for (auto &&[candidate, memberships] :
             prospective_parachains_->answerHypotheticalFrontierRequest(
                 std::span<const HypotheticalCandidate>{&hypothetical_candidate,
                                                        1},
                 {{head}},
                 backed_in_path_only)) {
          BOOST_ASSERT(candidateHash(candidate).get() == candidate_hash.get());
          for (auto &&[relay_parent, depths] : memberships) {
            BOOST_ASSERT(relay_parent == head);
            r.insert(r.end(), depths.begin(), depths.end());
          }
        }

        if (!proc_response(std::move(r), head, leaf_state)) {
          return std::nullopt;
        }
      } else {
        if (head == candidate_relay_parent.get()) {
          if (auto it = leaf_state.seconded_at_depth.find(candidate_para.get());
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
    REINVOKE(*main_pool_handler_,
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
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
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
        .commitments_hash = {},
        .prospective_candidate = std::move(prospective_candidate),
    };

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
            /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
            /// dequeue_next_collation_and_fetch
            return;
          }

          self->handleFetchedCollation(std::move(pending_collation),
                                       std::move(result).value());
        };

    SL_TRACE(logger_,
             "Requesting collation. (peer id={}, para id={}, relay parent={})",
             pc.peer_id,
             pc.para_id,
             pc.relay_parent);

    our_current_state_.collation_requests_cancel_handles.insert(std::move(pc));
    const auto maybe_candidate_hash =
        utils::map(pc.prospective_candidate,
                   [](const auto &pair) { return std::cref(pair.first); });
    per_relay_parent.collations.status = CollationStatus::Fetching;
    per_relay_parent.collations.fetching_from.emplace(id, maybe_candidate_hash);

    if (network::CollationVersion::V1 == version) {
      network::CollationFetchingRequest fetch_collation_request{
          .relay_parent = pc.relay_parent,
          .para_id = pc.para_id,
      };
      router_->getReqCollationProtocol()->request(
          peer_id,
          std::move(fetch_collation_request),
          std::move(response_callback));
    } else if (network::CollationVersion::VStaging == version
               && maybe_candidate_hash) {
      network::vstaging::CollationFetchingRequest fetch_collation_request{
          .relay_parent = pc.relay_parent,
          .para_id = pc.para_id,
          .candidate_hash = maybe_candidate_hash->get(),
      };
      router_->getReqCollationProtocol()->request(
          peer_id,
          std::move(fetch_collation_request),
          std::move(response_callback));
    } else {
      UNREACHABLE;
    }
  }

}  // namespace kagome::parachain

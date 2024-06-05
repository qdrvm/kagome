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
#include "consensus/babe/impl/babe_digests_util.hpp"
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

#ifndef TRY_GET_OR_RET
#define TRY_GET_OR_RET(name,op) \
  auto name = (op); \
  if (!op) { \
    return; \
  } else {}
#endif //TRY_GET_OR_RET

#ifndef CHECK_OR_RET
#define CHECK_OR_RET(op) \
  if(!(op)) { \
    return; \
  }
#endif //CHECK_OR_RET

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
    case E::CORE_INDEX_UNAVAILABLE:
      return "Core index unavailable";
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
    case E::INCORRECT_BITFIELD_SIZE:
      return "Incorrect bitfield size";
    case E::INCORRECT_SIGNATURE:
      return "Incorrect signature";
    case E::CLUSTER_TRACKER_ERROR:
      return "Cluster tracker error";
    case E::PERSISTED_VALIDATION_DATA_NOT_FOUND:
      return "Persisted validation data not found";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return "Persisted validation data mismatch";
    case E::CANDIDATE_HASH_MISMATCH:
      return "Candidate hash mismatch";
    case E::PARENT_HEAD_DATA_MISMATCH:
      return "Parent head data mismatch";
    case E::NO_PEER:
      return "No peer";
    case E::ALREADY_REQUESTED:
      return "Already requested";
    case E::NOT_ADVERTISED:
      return "Not advertised";
    case E::WRONG_PARA:
      return "Wrong para id";
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
      std::shared_ptr<ProspectiveParachains> prospective_parachains,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      LazySPtr<consensus::SlotsUtil> slots_util,
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo)
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
        per_session_(RefCache<SessionIndex, PerSessionState>::create()),
        slots_util_(std::move(slots_util)),
        babe_config_repo_(std::move(babe_config_repo)),
        chain_sub_{std::move(chain_sub_engine)},
        worker_pool_handler_{worker_thread_pool.handler(app_state_manager)},
        prospective_parachains_{std::move(prospective_parachains)},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(pm_);
    BOOST_ASSERT(peer_view_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(babe_config_repo_);
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
    BOOST_ASSERT(block_tree_);
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

    TRY_GET_OR_RET(relay_parent_state, tryGetStateByRelayParent(relay_parent));
    send_to_validators_group(
        relay_parent,
        {network::VersionedValidatorProtocolMessage{
            network::vstaging::ValidatorProtocolMessage{
                network::vstaging::BitfieldDistributionMessage{
                    network::vstaging::BitfieldDistribution{relay_parent,
                                                            bitfield}}}}});
  }

  /**
   * The `prepare` function is responsible for setting up the necessary
   * components for the `ParachainProcessorImpl` class. It sets up the broadcast
   * callback for the bitfield signer, subscribes to the BABE status observable,
   * chain events engine, and my view observable. It also prepares the active
   * leaves for processing parachains.
   * @return true if the preparation is successful.
   */
  bool ParachainProcessorImpl::prepare() {
    // Set the broadcast callback for the bitfield signer
    bitfield_signer_->setBroadcastCallback(
        [wptr_self{weak_from_this()}](const primitives::BlockHash &relay_parent,
                                      const network::SignedBitfield &bitfield) {
          if (auto self = wptr_self.lock()) {
            self->OnBroadcastBitfields(relay_parent, bitfield);
          }
        });

    // Subscribe to the BABE status observable
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
                TRY_GET_OR_RET(my_view, self->peer_view_->getMyView());
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

    // Subscribe to the chain events engine
    chain_sub_.onDeactivate(
        [wptr{weak_from_this()}](
            const primitives::events::RemoveAfterFinalizationParams &event) {
          if (auto self = wptr.lock()) {
            self->onDeactivateBlocks(event);
          }
        });

    // Set the callback for the my view observable
    // This callback is triggered when the kViewUpdate event is fired.
    // It updates the active leaves, checks if parachains can be processed,
    // creates a new backing task for the new head, and broadcasts the updated
    // view.
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
      const libp2p::peer::PeerId &peer, const network::View &new_view) {
    REINVOKE(*main_pool_handler_, onUpdatePeerView, peer, new_view);
    TRY_GET_OR_RET(peer_state, pm_->getPeerState(peer));

    auto fresh_implicit = peer_state->get().update_view(
        new_view, *our_current_state_.implicit_view);
    for (const auto &new_relay_parent : fresh_implicit) {
      send_peer_messages_for_relay_parent(peer, new_relay_parent);
    }
  }

  void ParachainProcessorImpl::send_pending_cluster_statements(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      ValidatorIndex peer_validator_id,
      ParachainProcessorImpl::RelayParentState &relay_parent_state) {
    CHECK_OR_RET(relay_parent_state.local_validator);
    CHECK_OR_RET(relay_parent_state.local_validator->active);

    const auto pending_statements =
        relay_parent_state.local_validator->active->cluster_tracker
            .pending_statements_for(peer_validator_id);
    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
        messages;
    for (const auto &[originator, compact] : pending_statements) {
      if (!candidates_.is_confirmed(candidateHash(compact))) {
        continue;
      }

      auto res =
          pending_statement_network_message(*relay_parent_state.statement_store,
                                            relay_parent,
                                            peer_id,
                                            version,
                                            originator,
                                            network::vstaging::from(compact));

      if (res) {
        relay_parent_state.local_validator->active->cluster_tracker.note_sent(
            peer_validator_id, originator, compact);
        messages.emplace_back(*res);
      }
    }

    auto se = pm_->getStreamEngine();
    for (auto &[peers, msg] : messages) {
      if (auto m = if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
        auto message = std::make_shared<
            network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
            std::move(m->get()));
        for (const auto &p : peers) {
          se->send(p, router_->getValidationProtocolVStaging(), message);
        }
      } else {
        BOOST_ASSERT(false);
      }
    }
  }

  void ParachainProcessorImpl::send_pending_grid_messages(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      ValidatorIndex peer_validator_id,
      const Groups &groups,
      ParachainProcessorImpl::RelayParentState &relay_parent_state) {
    CHECK_OR_RET(relay_parent_state.local_validator);

    auto pending_manifests =
        relay_parent_state.local_validator->grid_tracker.pending_manifests_for(
            peer_validator_id);
    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
        messages;
    for (const auto &[candidate_hash, kind] : pending_manifests) {
      const auto confirmed_candidate =
          candidates_.get_confirmed(candidate_hash);
      if (!confirmed_candidate) {
        continue;
      }

      const auto group_index = confirmed_candidate->get().group_index();
      TRY_GET_OR_RET(group, groups.get(group_index));

      const auto group_size = group->size();
      auto local_knowledge =
          local_knowledge_filter(group_size,
                                 group_index,
                                 candidate_hash,
                                 *relay_parent_state.statement_store);

      switch (kind) {
        case grid::ManifestKind::Full: {
          const network::vstaging::BackedCandidateManifest manifest{
              .relay_parent = relay_parent,
              .candidate_hash = candidate_hash,
              .group_index = group_index,
              .para_id = confirmed_candidate->get().para_id(),
              .parent_head_data_hash =
                  confirmed_candidate->get().parent_head_data_hash(),
              .statement_knowledge = local_knowledge,
          };

          auto &grid = relay_parent_state.local_validator->grid_tracker;
          grid.manifest_sent_to(
              groups, peer_validator_id, candidate_hash, local_knowledge);

          switch (version) {
            case network::CollationVersion::VStaging: {
              messages.emplace_back(
                  std::vector<libp2p::peer::PeerId>{peer_id},
                  network::VersionedValidatorProtocolMessage{
                      kagome::network::vstaging::ValidatorProtocolMessage{
                          kagome::network::vstaging::
                              StatementDistributionMessage{manifest}}});
            } break;
            default: {
              SL_ERROR(logger_,
                       "Bug ValidationVersion::V1 should not be used in "
                       "statement-distribution v2, legacy should have handled "
                       "this.");
            } break;
          };
        } break;
        case grid::ManifestKind::Acknowledgement: {
          auto m = acknowledgement_and_statement_messages(
              peer_id,
              network::CollationVersion::VStaging,
              peer_validator_id,
              groups,
              relay_parent_state,
              relay_parent,
              group_index,
              candidate_hash,
              local_knowledge);
          messages.insert(messages.end(),
                          std::move_iterator(m.begin()),
                          std::move_iterator(m.end()));

        } break;
      }
    }

    {
      auto &grid_tracker = relay_parent_state.local_validator->grid_tracker;
      auto pending_statements =
          grid_tracker.all_pending_statements_for(peer_validator_id);

      for (const auto &[originator, compact] : pending_statements) {
        auto res = pending_statement_network_message(
            *relay_parent_state.statement_store,
            relay_parent,
            peer_id,
            network::CollationVersion::VStaging,
            originator,
            compact);

        if (res) {
          grid_tracker.sent_or_received_direct_statement(
              groups, originator, peer_validator_id, compact, false);

          messages.emplace_back(std::move(*res));
        }
      }
    }

    auto se = pm_->getStreamEngine();
    for (auto &[peers, msg] : messages) {
      if (auto m = if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
        auto message = std::make_shared<
            network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
            std::move(m->get()));
        for (const auto &p : peers) {
          se->send(p, router_->getValidationProtocolVStaging(), message);
        }
      } else {
        assert(false);
      }
    }
  }

  std::optional<std::pair<std::vector<libp2p::peer::PeerId>,
                          network::VersionedValidatorProtocolMessage>>
  ParachainProcessorImpl::pending_statement_network_message(
      const StatementStore &statement_store,
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer,
      network::CollationVersion version,
      ValidatorIndex originator,
      const network::vstaging::CompactStatement &compact) {
    switch (version) {
      case network::CollationVersion::VStaging: {
        auto s = statement_store.validator_statement(originator, compact);
        if (s) {
          return std::make_pair(
              std::vector<libp2p::peer::PeerId>{peer},
              network::VersionedValidatorProtocolMessage{
                  network::vstaging::ValidatorProtocolMessage{
                      network::vstaging::StatementDistributionMessage{
                          network::vstaging::
                              StatementDistributionMessageStatement{
                                  .relay_parent = relay_parent,
                                  .compact = s->get().statement,
                              }}}});
        }
      } break;
      default: {
        SL_ERROR(logger_,
                 "Bug ValidationVersion::V1 should not be used in "
                 "statement-distribution v2, legacy should have handled this");
      } break;
    }
    return {};
  }

  void ParachainProcessorImpl::send_peer_messages_for_relay_parent(
      const libp2p::peer::PeerId &peer_id, const RelayHash &relay_parent) {
    BOOST_ASSERT(
        main_pool_handler_
            ->isInCurrentThread());  // because of pm_->getPeerState(...)

    TRY_GET_OR_RET(peer_state, pm_->getPeerState(peer_id));
    TRY_GET_OR_RET(parachain_state, tryGetStateByRelayParent(relay_parent));

    network::CollationVersion version = network::CollationVersion::VStaging;
    if (peer_state->get().version) {
      version = *peer_state->get().version;
    }

    if (auto auth_id = query_audi_->get(peer_id)) {
      if (auto it = parachain_state->get().authority_lookup.find(*auth_id);
          it != parachain_state->get().authority_lookup.end()) {
        ValidatorIndex vi = it->second;

        send_pending_cluster_statements(
            relay_parent, peer_id, version, vi, parachain_state->get());

        send_pending_grid_messages(
            relay_parent,
            peer_id,
            version,
            vi,
            parachain_state->get().per_session_state->value().groups,
            parachain_state->get());
      }
    }
  }

  void ParachainProcessorImpl::onViewUpdated(const network::ExView &event) {
    REINVOKE(*main_pool_handler_, onViewUpdated, event);
    CHECK_OR_RET(canProcessParachains().has_value());

    const auto &relay_parent = event.new_head.hash();
    if (const auto r =
            prospective_parachains_->onActiveLeavesUpdate(network::ExViewRef{
                .new_head = {event.new_head},
                .lost = event.lost,
            });
        r.has_error()) {
      SL_WARN(logger_,
              "Prospective parachains leaf update failed. (relay_parent={}, "
              "error={})",
              relay_parent,
              r.error().message());
    }

    backing_store_->onActivateLeaf(relay_parent);
    createBackingTask(relay_parent, event.new_head);
    SL_TRACE(logger_,
             "Update my view.(new head={}, finalized={}, leaves={})",
             relay_parent,
             event.view.finalized_number_,
             event.view.heads_.size());
    broadcastView(event.view);
    broadcastViewToGroup(relay_parent, event.view);

    {
      auto new_relay_parents =
          our_current_state_.implicit_view->all_allowed_relay_parents();
      std::vector<std::pair<libp2p::peer::PeerId, std::vector<Hash>>>
          update_peers;
      pm_->enumeratePeerState([&](const libp2p::peer::PeerId &peer,
                                  network::PeerState &peer_state) {
        std::vector<Hash> fresh =
            peer_state.reconcile_active_leaf(relay_parent, new_relay_parents);
        if (!fresh.empty()) {
          update_peers.emplace_back(peer, fresh);
        }
        return true;
      });
      for (const auto &[peer, fresh] : update_peers) {
        for (const auto &fresh_relay_parent : fresh) {
          send_peer_messages_for_relay_parent(peer, fresh_relay_parent);
        }
      }
    }
    new_leaf_fragment_tree_updates(relay_parent);

    // need to lock removing session infoes
    std::vector<
        std::shared_ptr<RefCache<SessionIndex, PerSessionState>::RefObj>>
        _keeper_;
    _keeper_.reserve(event.lost.size());

    for (const auto &lost : event.lost) {
      SL_TRACE(logger_, "Removed backing task.(relay parent={})", lost);
      auto relay_parent_state = tryGetStateByRelayParent(lost);
      if (relay_parent_state) {
        _keeper_.emplace_back(relay_parent_state->get().per_session_state);
      }

      our_current_state_.active_leaves.erase(lost);

      std::vector<Hash> pruned =
          our_current_state_.implicit_view->deactivate_leaf(lost);
      for (const auto removed : pruned) {
        our_current_state_.state_by_relay_parent.erase(removed);
        /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
        /// fetched_candidates ???
      }

      {  /// remove cancelations
        auto &container = our_current_state_.collation_requests_cancel_handles;
        for (auto pc = container.begin(); pc != container.end();) {
          if (pc->relay_parent != lost) {
            ++pc;
          } else {
            pc = container.erase(pc);
          }
        }
      }
      {  /// remove fetched candidates
        auto &container = our_current_state_.validator_side.fetched_candidates;
        for (auto pc = container.begin(); pc != container.end();) {
          if (pc->first.relay_parent != lost) {
            ++pc;
          } else {
            pc = container.erase(pc);
          }
        }
      }

      our_current_state_.per_leaf.erase(lost);
      our_current_state_.state_by_relay_parent.erase(lost);
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
      if (auto r = block_tree_->getBlockHeader(maybe_new); r.has_value()) {
        createBackingTask(maybe_new, r.value());
      } else {
        SL_ERROR(logger_, "No header found.(relay parent={})", maybe_new);
      }
    }

    auto remove_if = [](bool eq, auto &it, auto &cont) {
      if (eq) {
        it = cont.erase(it);
      } else {
        ++it;
      }
    };

    // para
    for (auto it = our_current_state_.blocked_advertisements.begin();
         it != our_current_state_.blocked_advertisements.end();) {
      // hash
      for (auto it_2 = it->second.begin(); it_2 != it->second.end();) {
        // adv
        for (auto it_3 = it_2->second.begin(); it_3 != it_2->second.end();) {
          remove_if(!tryGetStateByRelayParent(it_3->candidate_relay_parent),
                    it_3,
                    it_2->second);
        }
        remove_if(it_2->second.empty(), it_2, it->second);
      }
      remove_if(
          it->second.empty(), it, our_current_state_.blocked_advertisements);
    }

    auto maybe_unblocked = std::move(our_current_state_.blocked_advertisements);
    requestUnblockedCollations(std::move(maybe_unblocked));

    prune_old_advertisements(*our_current_state_.implicit_view,
                             our_current_state_.active_leaves,
                             our_current_state_.state_by_relay_parent);
  }

  void ParachainProcessorImpl::onDeactivateBlocks(
      const primitives::events::RemoveAfterFinalizationParams &event) {
    REINVOKE(*main_pool_handler_, onDeactivateBlocks, event);

    for (const auto &lost : event) {
      SL_TRACE(logger_, "Remove from storages.(relay parent={})", lost);

      backing_store_->onDeactivateLeaf(lost);
      av_store_->remove(lost);
      bitfield_store_->remove(lost);
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
    TRY_GET_OR_RET(opt_parachain_state, tryGetStateByRelayParent(relay_parent));

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

  void ParachainProcessorImpl::spawn_and_update_peer(
      const primitives::AuthorityDiscoveryId &id) {
    if (auto peer = query_audi_->get(id)) {
      tryOpenOutgoingValidationStream(
          peer->id,
          network::CollationVersion::VStaging,
          [wptr{weak_from_this()}, peer_id{peer->id}](auto &&stream) {
            if (auto self = wptr.lock()) {
              auto ps = self->pm_->getPeerState(peer_id);
              BOOST_ASSERT(ps);

              ps->get().inc_use_count();
              self->sendMyView(peer_id,
                               stream,
                               self->router_->getValidationProtocolVStaging());
            }
          });
    }
  }

  ParachainProcessorImpl::PerSessionState::PerSessionState(
      SessionIndex _session,
      const runtime::SessionInfo &_session_info,
      Groups &&_groups,
      grid::Views &&_grid_view,
      ValidatorIndex _our_index,
      const std::shared_ptr<network::PeerManager> &_pm,
      const std::shared_ptr<authority_discovery::Query> &_query_audi)
      : session{_session},
        session_info{_session_info},
        groups{std::move(_groups)},
        grid_view{std::move(_grid_view)},
        our_index{_our_index},
        pm{_pm},
        query_audi{_query_audi} {}

  ParachainProcessorImpl::PerSessionState::~PerSessionState() {
    if (our_index && grid_view) {
      if (auto our_group = groups.byValidatorIndex(*our_index)) {
        BOOST_ASSERT(*our_group < session_info.validator_groups.size());
        const auto &group = session_info.validator_groups[*our_group];

        auto dec_use_count_for_peer = [&](ValidatorIndex vi) {
          if (auto peer = query_audi->get(session_info.discovery_keys[vi])) {
            auto ps = pm->getPeerState(peer->id);
            BOOST_ASSERT(ps);
            ps->get().dec_use_count();
          }
        };

        /// update peers of our group
        for (const auto vi : group) {
          dec_use_count_for_peer(vi);
        }

        /// update peers in grid view
        if (grid_view) {
          BOOST_ASSERT(*our_group < grid_view->size());
          const auto &view = (*grid_view)[*our_group];
          for (const auto vi : view.sending) {
            dec_use_count_for_peer(vi);
          }
          for (const auto vi : view.receiving) {
            dec_use_count_for_peer(vi);
          }
        }
      }
    }
  }

  outcome::result<std::optional<runtime::ClaimQueueSnapshot>>
  ParachainProcessorImpl::fetch_claim_queue(const RelayHash &relay_parent) {
    constexpr uint32_t CLAIM_QUEUE_RUNTIME_REQUIREMENT = 11;
    OUTCOME_TRY(version, parachain_host_->runtime_api_version(relay_parent));
    if (version < CLAIM_QUEUE_RUNTIME_REQUIREMENT) {
      SL_TRACE(logger_, "Runtime doesn't support `request_claim_queue`");
      return std::nullopt;
    }

    OUTCOME_TRY(claims, parachain_host_->claim_queue(relay_parent));
    return runtime::ClaimQueueSnapshot{
        .claimes = std::move(claims),
    };
  }

  outcome::result<consensus::Randomness>
  ParachainProcessorImpl::getBabeRandomness(
      const primitives::BlockHeader &block_header) {
    OUTCOME_TRY(babe_header, consensus::babe::getBabeBlockHeader(block_header));
    OUTCOME_TRY(epoch,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               babe_header.slot_number));
    OUTCOME_TRY(babe_config,
                babe_config_repo_->config(*block_header.parentInfo(), epoch));

    return babe_config->randomness;
  }

  outcome::result<kagome::parachain::ParachainProcessorImpl::RelayParentState>
  ParachainProcessorImpl::initNewBackingTask(
      const primitives::BlockHash &relay_parent,
      const network::HashedBlockHeader &block_header) {
    /**
     * It first checks if our node is a parachain validator for the relay
     * parent. If it is not, it returns an error. If the node is a validator, it
     * retrieves the validator groups, availability cores, and validators for
     * the relay parent. It then iterates over the cores and for each scheduled
     * core, it checks if the node is part of the validator group for that core.
     * If it is, it assigns the parachain ID and collator ID of the scheduled
     * core to the node. It also maps the parachain ID to the validators of the
     * group. Finally, it returns a `RelayParentState` object that contains the
     * assignment, validator index, required collator, and table context.
     */
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
    OUTCOME_TRY(randomness, getBabeRandomness(block_header));
    const auto &[validator_groups, group_rotation_info] = groups;

    if (!validator) {
      SL_TRACE(logger_, "Not a validator, or no para keys.");
      return Error::KEY_NOT_PRESENT;
    }
    is_parachain_validator = true;

    if (!session_info) {
      return Error::NO_SESSION_INFO;
    }

    bool inject_core_index = false;
    if (auto r = parachain_host_->node_features(relay_parent, session_index);
        r.has_value()) {
      if (r.value()
          && r.value()->bits.size() > runtime::ParachainHost::NodeFeatureIndex::
                     ElasticScalingMVP) {
        inject_core_index =
            r.value()->bits
                [runtime::ParachainHost::NodeFeatureIndex::ElasticScalingMVP];
      }
    }

    uint32_t minimum_backing_votes = 2;  /// legacy value
    if (auto r =
            parachain_host_->minimum_backing_votes(relay_parent, session_index);
        r.has_value()) {
      minimum_backing_votes = r.value();
    } else {
      SL_TRACE(logger_,
               "Querying the backing threshold from the runtime is not "
               "supported by the current Runtime API. (relay_parent={})",
               relay_parent);
    }

    auto per_session_state = per_session_->get_or_insert(session_index, [&]() {
      const auto validator_index{validator->validatorIndex()};
      SL_TRACE(
          logger_, "===> Grid build (validator_index={})", validator_index);

      grid::Views grid_view = grid::makeViews(
          session_info->validator_groups,
          grid::shuffle(session_info->validator_groups, randomness),
          validator_index);

      return RefCache<SessionIndex, PerSessionState>::RefObj(
          session_index,
          *session_info,
          Groups{session_info->validator_groups, minimum_backing_votes},
          std::move(grid_view),
          validator_index,
          pm_,
          query_audi_);
    });

    if (auto our_group = per_session_state->value().groups.byValidatorIndex(
            validator->validatorIndex())) {
      /// update peers of our group
      const auto &group = session_info->validator_groups[*our_group];
      for (const auto vi : group) {
        spawn_and_update_peer(session_info->discovery_keys[vi]);
      }

      /// update peers in grid view
      const auto &grid_view = *per_session_state->value().grid_view;
      BOOST_ASSERT(*our_group < grid_view.size());
      const auto &view = grid_view[*our_group];
      for (const auto vi : view.sending) {
        spawn_and_update_peer(session_info->discovery_keys[vi]);
      }
      for (const auto vi : view.receiving) {
        spawn_and_update_peer(session_info->discovery_keys[vi]);
      }
    }

    auto mode =
        prospective_parachains_->prospectiveParachainsMode(relay_parent);
    BOOST_ASSERT(mode);
    if (!mode) {
      SL_ERROR(logger_,
               "Prospective parachains are disabled. No sure for correctness");
    }
    const auto n_cores = cores.size();

    std::unordered_map<CoreIndex, std::vector<ValidatorIndex>> out_groups;
    std::optional<CoreIndex> assigned_core;
    std::optional<ParachainId> assigned_para;

    for (CoreIndex idx = 0; idx < static_cast<CoreIndex>(cores.size()); ++idx) {
      std::optional<ParachainId> core_para_id = visit_in_place(
          cores[idx],
          [&](const runtime::OccupiedCore &occupied)
              -> std::optional<ParachainId> {
            if (mode && occupied.next_up_on_available) {
              return occupied.next_up_on_available->para_id;
            }
            return std::nullopt;
          },
          [](const runtime::ScheduledCore &scheduled)
              -> std::optional<ParachainId> { return scheduled.para_id; },
          [](const runtime::FreeCore &) -> std::optional<ParachainId> {
            return std::nullopt;
          });

      if (!core_para_id) {
        continue;
      }

      const CoreIndex core_index = idx;
      const GroupIndex group_index =
          group_rotation_info.groupForCore(core_index, n_cores);

      if (group_index < validator_groups.size()) {
        const auto &g = validator_groups[group_index];
        if (validator && g.contains(validator->validatorIndex())) {
          assigned_para = core_para_id;
          assigned_core = core_index;
        }
        out_groups.emplace(core_index, g.validators);
      }
    }

    std::vector<std::optional<GroupIndex>> validator_to_group;
    validator_to_group.resize(validators.size());
    for (GroupIndex group_idx = 0; group_idx < validator_groups.size();
         ++group_idx) {
      const auto &validator_group = validator_groups[group_idx];
      for (const auto v : validator_group.validators) {
        SL_TRACE(logger_, "Bind {} -> {}", v, group_idx);
        validator_to_group[v] = group_idx;
      }
    }

    std::unordered_map<primitives::AuthorityDiscoveryId, ValidatorIndex>
        authority_lookup;
    for (ValidatorIndex v = 0;
         v < per_session_state->value().session_info.discovery_keys.size();
         ++v) {
      authority_lookup[per_session_state->value()
                           .session_info.discovery_keys[v]] = v;
    }

    std::optional<StatementStore> statement_store;
    if (mode) {
      [[maybe_unused]] const auto _ =
          our_current_state_.implicit_view->activate_leaf(relay_parent);
      statement_store.emplace(per_session_state->value().groups);
    }

    auto maybe_claim_queue =
        [&]() -> std::optional<runtime::ClaimQueueSnapshot> {
      auto r = fetch_claim_queue(relay_parent);
      if (r.has_value()) {
        return r.value();
      }
      return std::nullopt;
    }();

    const auto seconding_limit = mode->max_candidate_depth + 1;
    std::optional<LocalValidatorState> local_validator =
        find_active_validator_state(validator->validatorIndex(),
                                    per_session_state->value().groups,
                                    cores,
                                    group_rotation_info,
                                    maybe_claim_queue,
                                    seconding_limit,
                                    mode->max_candidate_depth);

    SL_VERBOSE(logger_,
               "Inited new backing task v3.(assigned_para={}, "
               "assigned_core={}, our index={}, relay "
               "parent={})",
               assigned_para,
               assigned_core,
               validator->validatorIndex(),
               relay_parent);

    return RelayParentState{
        .prospective_parachains_mode = mode,
        .assigned_core = assigned_core,
        .assigned_para = assigned_para,
        .validator_to_group = std::move(validator_to_group),
        .per_session_state = per_session_state,
        .our_index = validator->validatorIndex(),
        .our_group = per_session_state->value().groups.byValidatorIndex(
            validator->validatorIndex()),
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
        .authority_lookup = std::move(authority_lookup),
        .local_validator = local_validator,
        .awaiting_validation = {},
        .issued_statements = {},
        .peers_advertised = {},
        .fallbacks = {},
        .inject_core_index = inject_core_index,
    };
  }

  std::optional<ParachainProcessorImpl::LocalValidatorState>
  ParachainProcessorImpl::find_active_validator_state(
      ValidatorIndex validator_index,
      const Groups &groups,
      const std::vector<runtime::CoreState> &availability_cores,
      const runtime::GroupDescriptor &group_rotation_info,
      const std::optional<runtime::ClaimQueueSnapshot> &maybe_claim_queue,
      size_t seconding_limit,
      size_t max_candidate_depth) {
    if (groups.all_empty()) {
      return std::nullopt;
    }

    const auto our_group = groups.byValidatorIndex(validator_index);
    if (!our_group) {
      return std::nullopt;
    }

    const auto core_index =
        group_rotation_info.coreForGroup(*our_group, availability_cores.size());
    std::optional<ParachainId> para_assigned_to_core;
    if (maybe_claim_queue) {
      para_assigned_to_core = maybe_claim_queue->get_claim_for(core_index, 0);
    } else {
      if (core_index < availability_cores.size()) {
        const auto &core_state = availability_cores[core_index];
        visit_in_place(
            core_state,
            [&](const runtime::ScheduledCore &scheduled) {
              para_assigned_to_core = scheduled.para_id;
            },
            [&](const runtime::OccupiedCore &occupied) {
              if (max_candidate_depth >= 1 && occupied.next_up_on_available) {
                para_assigned_to_core = occupied.next_up_on_available->para_id;
              }
            },
            [](const auto &) {});
      }
    }

    const auto group_validators = groups.get(*our_group);
    if (!group_validators) {
      return std::nullopt;
    }

    return LocalValidatorState{
        .grid_tracker = {},
        .active =
            ActiveValidatorState{
                .index = validator_index,
                .group = *our_group,
                .assignment = para_assigned_to_core,
                .cluster_tracker = ClusterTracker(
                    {group_validators->begin(), group_validators->end()},
                    seconding_limit),
            },
    };
  }

  void ParachainProcessorImpl::createBackingTask(
      const primitives::BlockHash &relay_parent,
      const network::HashedBlockHeader &block_header) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    auto rps_result = initNewBackingTask(relay_parent, block_header);
    if (rps_result.has_value()) {
      storeStateByRelayParent(relay_parent, std::move(rps_result.value()));
    } else if (rps_result.error() != Error::KEY_NOT_PRESENT) {
      SL_TRACE(
          logger_,
          "Relay parent state was not created. (relay parent={}, error={})",
          relay_parent,
          rps_result.error());
    }
  }

  void ParachainProcessorImpl::handle_collation_fetch_response(
      network::CollationEvent &&collation_event,
      network::CollationFetchingResponse &&response) {
    REINVOKE(*main_pool_handler_,
             handle_collation_fetch_response,
             std::move(collation_event),
             std::move(response));

    const auto &pending_collation = collation_event.pending_collation;
    SL_TRACE(logger_,
             "Processing collation from {}, relay parent: {}, para id: {}",
             pending_collation.peer_id,
             pending_collation.relay_parent,
             pending_collation.para_id);

    our_current_state_.collation_requests_cancel_handles.erase(
        pending_collation);

    outcome::result<network::PendingCollationFetch> p = visit_in_place(
        std::move(response.response_data),
        [&](network::CollationResponse &&value)
            -> outcome::result<network::PendingCollationFetch> {
          if (value.receipt.descriptor.para_id != pending_collation.para_id) {
            SL_TRACE(logger_,
                     "Got wrong para ID for requested collation. "
                     "(expected_para_id={}, got_para_id={}, peer_id={})",
                     pending_collation.para_id,
                     value.receipt.descriptor.para_id,
                     pending_collation.peer_id);
            return Error::WRONG_PARA;
          }

          SL_TRACE(logger_,
                   "Received collation (para_id={}, relay_parent={}, "
                   "candidate_hash={})",
                   pending_collation.para_id,
                   pending_collation.relay_parent,
                   value.receipt.hash(*hasher_));

          return network::PendingCollationFetch{
              .collation_event = std::move(collation_event),
              .candidate_receipt = std::move(value.receipt),
              .pov = std::move(value.pov),
              .maybe_parent_head_data = std::nullopt,
          };
        },
        [&](network::CollationWithParentHeadData &&value)
            -> outcome::result<network::PendingCollationFetch> {
          if (value.receipt.descriptor.para_id != pending_collation.para_id) {
            SL_TRACE(logger_,
                     "Got wrong para ID for requested collation (v3). "
                     "(expected_para_id={}, got_para_id={}, peer_id={})",
                     pending_collation.para_id,
                     value.receipt.descriptor.para_id,
                     pending_collation.peer_id);
            return Error::WRONG_PARA;
          }

          SL_TRACE(logger_,
                   "Received collation (v3) (para_id={}, relay_parent={}, "
                   "candidate_hash={})",
                   pending_collation.para_id,
                   pending_collation.relay_parent,
                   value.receipt.hash(*hasher_));

          return network::PendingCollationFetch{
              .collation_event = std::move(collation_event),
              .candidate_receipt = std::move(value.receipt),
              .pov = std::move(value.pov),
              .maybe_parent_head_data = std::move(value.parent_head_data),
          };
        });

    if (p.has_error()) {
      SL_TRACE(logger_, "Collation process failed (error={})", p.error());
      return;
    }

    CollatorId collator_id = p.value().collation_event.collator_id;
    PendingCollation pending_collation_copy =
        p.value().collation_event.pending_collation;

    if (auto res = kick_off_seconding(std::move(p.value())); res.has_error()) {
      SL_WARN(logger_,
              "Seconding aborted due to an error. (relay_parent={}, "
              "para_id={}, peer_id={}, error={})",
              pending_collation_copy.relay_parent,
              pending_collation_copy.para_id,
              pending_collation_copy.peer_id,
              res.error());

      const auto maybe_candidate_hash =
          utils::map(pending_collation_copy.prospective_candidate,
                     [](const auto &v) { return v.candidate_hash; });
      dequeue_next_collation_and_fetch(pending_collation_copy.relay_parent,
                                       {collator_id, maybe_candidate_hash});
    }
  }

  outcome::result<void> ParachainProcessorImpl::fetched_collation_sanity_check(
      const PendingCollation &advertised,
      const network::CandidateReceipt &fetched,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>>
          &persisted_validation_data,
      std::optional<std::pair<HeadData, Hash>> maybe_parent_head_and_hash) {
    if (persisted_validation_data.getHash()
        != fetched.descriptor.persisted_data_hash) {
      return Error::PERSISTED_VALIDATION_DATA_MISMATCH;
    }

    if (advertised.prospective_candidate
        && advertised.prospective_candidate->candidate_hash
               != fetched.hash(*hasher_)) {
      return Error::CANDIDATE_HASH_MISMATCH;
    }

    if (maybe_parent_head_and_hash
        && hasher_->blake2b_256(maybe_parent_head_and_hash->first)
               != maybe_parent_head_and_hash->second) {
      return Error::PARENT_HEAD_DATA_MISMATCH;
    }

    return outcome::success();
  }

  void ParachainProcessorImpl::dequeue_next_collation_and_fetch(
      const RelayHash &relay_parent,
      std::pair<CollatorId, std::optional<CandidateHash>> previous_fetch) {
    auto per_relay_state = tryGetStateByRelayParent(relay_parent);
    if (!per_relay_state) {
      return;
    }

    while (auto collation =
               per_relay_state->get().collations.get_next_collation_to_fetch(
                   previous_fetch,
                   per_relay_state->get().prospective_parachains_mode,
                   logger_)) {
      const auto &[next, id] = std::move(*collation);
      SL_TRACE(logger_,
               "Successfully dequeued next advertisement - fetching ... "
               "(relay_parent={}, id={})",
               relay_parent,
               id);
      if (auto res = fetchCollation(next, id); res.has_error()) {
        SL_TRACE(logger_,
                 "Failed to request a collation, dequeueing next one "
                 "(relay_parent={}, para_id={}, peer_id={}, error={})",
                 next.relay_parent,
                 next.para_id,
                 next.peer_id,
                 res.error());
      } else {
        break;
      }
    }
  }

  outcome::result<std::optional<runtime::PersistedValidationData>>
  ParachainProcessorImpl::requestProspectiveValidationData(
      const RelayHash &candidate_relay_parent,
      const Hash &parent_head_data_hash,
      ParachainId para_id,
      const std::optional<HeadData> &maybe_parent_head_data) {
    auto parent_head_data = [&]() -> ParentHeadData {
      if (maybe_parent_head_data) {
        return ParentHeadData_WithData{*maybe_parent_head_data,
                                       parent_head_data_hash};
      }
      return parent_head_data_hash;
    }();

    OUTCOME_TRY(opt_pvd,
                prospective_parachains_->answerProspectiveValidationDataRequest(
                    candidate_relay_parent, parent_head_data, para_id));
    return opt_pvd;
  }

  outcome::result<std::optional<runtime::PersistedValidationData>>
  ParachainProcessorImpl::fetchPersistedValidationData(
      const RelayHash &relay_parent, ParachainId para_id) {
    return requestPersistedValidationData(relay_parent, para_id);
  }

  outcome::result<std::optional<runtime::PersistedValidationData>>
  ParachainProcessorImpl::requestPersistedValidationData(
      const RelayHash &relay_parent, ParachainId para_id) {
    OUTCOME_TRY(
        pvd,
        parachain_host_->persisted_validation_data(
            relay_parent, para_id, runtime::OccupiedCoreAssumption::Free));
    return pvd;
  }

  void ParachainProcessorImpl::process_bitfield_distribution(
      const network::BitfieldDistributionMessage &val) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    auto bd{boost::get<const network::BitfieldDistribution>(&val)};
    BOOST_ASSERT_MSG(
        bd, "BitfieldDistribution is not present. Check message format.");

    SL_TRACE(logger_,
             "Incoming `BitfieldDistributionMessage`. (relay_parent={})",
             bd->relay_parent);
    auto parachain_state = tryGetStateByRelayParent(bd->relay_parent);
    if (!parachain_state) {
      return;
    }

    const auto &session_info =
        parachain_state->get().per_session_state->value().session_info;
    if (bd->data.payload.ix >= session_info.validators.size()) {
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

    auto res =
        crypto_provider_->verify(bd->data.signature,
                                 buffer,
                                 session_info.validators[bd->data.payload.ix]);
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
      ParachainId para_id,
      grid::ManifestKind manifest_kind) {
    auto peer_state = pm_->getPeerState(peer_id);
    if (!peer_state) {
      SL_WARN(logger_, "No peer state. (peer_id={})", peer_id);
      return {};
    }

    auto relay_parent_state = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state) {
      return {};
    }

    if (!relay_parent_state->get().local_validator) {
      return {};
    }

    auto expected_group =
        group_for_para(relay_parent_state->get().availability_cores,
                       relay_parent_state->get().group_rotation_info,
                       para_id);

    if (!expected_group
        || *expected_group != manifest_summary.claimed_group_index) {
      return {};
    }

    if (!relay_parent_state->get().per_session_state->value().grid_view) {
      return {};
    }

    const auto &grid_topology =
        *relay_parent_state->get().per_session_state->value().grid_view;
    if (manifest_summary.claimed_group_index >= grid_topology.size()) {
      return {};
    }

    auto sender_index = [&]() -> std::optional<ValidatorIndex> {
      const auto &sub = grid_topology[manifest_summary.claimed_group_index];
      const auto &iter = (manifest_kind == grid::ManifestKind::Full)
                           ? sub.receiving
                           : sub.sending;
      if (!iter.empty()) {
        return *iter.begin();
      }
      return {};
    }();

    if (!sender_index) {
      return {};
    }

    auto group_index = manifest_summary.claimed_group_index;
    auto claimed_parent_hash = manifest_summary.claimed_parent_hash;

    /// TODO(iceseer): do `disabled validators`
    /// https://github.com/qdrvm/kagome/issues/2060

    BOOST_ASSERT(relay_parent_state->get().prospective_parachains_mode);
    const auto seconding_limit =
        relay_parent_state->get()
            .prospective_parachains_mode->max_candidate_depth
        + 1;

    auto &local_validator = *relay_parent_state->get().local_validator;
    auto acknowledge_res = local_validator.grid_tracker.import_manifest(
        grid_topology,
        relay_parent_state->get().per_session_state->value().groups,
        candidate_hash,
        seconding_limit,
        manifest_summary,
        manifest_kind,
        *sender_index);

    if (acknowledge_res.has_error()) {
      return {};
    }

    const auto acknowledge = acknowledge_res.value();
    if (!candidates_.insert_unconfirmed(peer_id,
                                        candidate_hash,
                                        relay_parent,
                                        group_index,
                                        {{claimed_parent_hash, para_id}})) {
      SL_TRACE(logger_,
               "Insert unconfirmed candidate failed. (candidate hash={}, relay "
               "parent={}, para id={}, claimed parent={})",
               candidate_hash,
               relay_parent,
               para_id,
               manifest_summary.claimed_parent_hash);
      return {};
    }

    if (acknowledge) {
      SL_TRACE(logger_,
               "immediate ack, known candidate. (candidate hash={}, from={}, "
               "local_validator={})",
               candidate_hash,
               *sender_index,
               *relay_parent_state->get().our_index);
    }

    return ManifestImportSuccess{
        .acknowledge = acknowledge,
        .sender_index = *sender_index,
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

  std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                       network::VersionedValidatorProtocolMessage>>
  ParachainProcessorImpl::acknowledgement_and_statement_messages(
      const libp2p::peer::PeerId &peer,
      network::CollationVersion version,
      ValidatorIndex validator_index,
      const Groups &groups,
      ParachainProcessorImpl::RelayParentState &relay_parent_state,
      const RelayHash &relay_parent,
      GroupIndex group_index,
      const CandidateHash &candidate_hash,
      const network::vstaging::StatementFilter &local_knowledge) {
    if (!relay_parent_state.local_validator) {
      return {};
    }

    auto &local_validator = *relay_parent_state.local_validator;
    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
        messages;

    switch (version) {
      case network::CollationVersion::VStaging: {
        messages.emplace_back(
            std::vector<libp2p::peer::PeerId>{peer},
            network::VersionedValidatorProtocolMessage{
                network::vstaging::ValidatorProtocolMessage{
                    network::vstaging::StatementDistributionMessage{
                        network::vstaging::BackedCandidateAcknowledgement{
                            .candidate_hash = candidate_hash,
                            .statement_knowledge = local_knowledge,
                        }}}});
      } break;
      default: {
        SL_ERROR(logger_,
                 "Bug ValidationVersion::V1 should not be used in "
                 "statement-distribution v2, legacy should have handled this");
        return {};
      } break;
    };

    local_validator.grid_tracker.manifest_sent_to(
        groups, validator_index, candidate_hash, local_knowledge);

    auto statement_messages = post_acknowledgement_statement_messages(
        validator_index,
        relay_parent,
        local_validator.grid_tracker,
        *relay_parent_state.statement_store,
        groups,
        group_index,
        candidate_hash,
        peer,
        version);

    for (auto &&m : statement_messages) {
      messages.emplace_back(std::vector<libp2p::peer::PeerId>{peer},
                            std::move(m));
    }
    return messages;
  }

  std::deque<network::VersionedValidatorProtocolMessage>
  ParachainProcessorImpl::post_acknowledgement_statement_messages(
      ValidatorIndex recipient,
      const RelayHash &relay_parent,
      grid::GridTracker &grid_tracker,
      const StatementStore &statement_store,
      const Groups &groups,
      GroupIndex group_index,
      const CandidateHash &candidate_hash,
      const libp2p::peer::PeerId &peer,
      network::CollationVersion version) {
    auto sending_filter =
        grid_tracker.pending_statements_for(recipient, candidate_hash);
    if (!sending_filter) {
      return {};
    }

    std::deque<network::VersionedValidatorProtocolMessage> messages;
    auto group = groups.get(group_index);
    if (!group) {
      return messages;
    }

    statement_store.groupStatements(
        *group,
        candidate_hash,
        *sending_filter,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) {
          grid_tracker.sent_or_received_direct_statement(groups,
                                                         statement.payload.ix,
                                                         recipient,
                                                         getPayload(statement),
                                                         false);

          switch (version) {
            case network::CollationVersion::VStaging: {
              messages.emplace_back(network::VersionedValidatorProtocolMessage{
                  network::vstaging::ValidatorProtocolMessage{
                      network::vstaging::StatementDistributionMessage{
                          network::vstaging::
                              StatementDistributionMessageStatement{
                                  .relay_parent = relay_parent,
                                  .compact = statement,
                              }}}});
            } break;
            default: {
              SL_ERROR(
                  logger_,
                  "Bug ValidationVersion::V1 should not be used in "
                  "statement-distribution v2, legacy should have handled this");
            } break;
          }
        });
    return messages;
  }

  outcome::result<void> ParachainProcessorImpl::handle_grid_statement(
      const RelayHash &relay_parent,
      ParachainProcessorImpl::RelayParentState &per_relay_parent,
      grid::GridTracker &grid_tracker,
      const IndexedAndSigned<network::vstaging::CompactStatement> &statement,
      ValidatorIndex grid_sender_index) {
    /// TODO(iceseer): do Ensure the statement is correctly signed. Signature
    /// check.
    grid_tracker.sent_or_received_direct_statement(
        per_relay_parent.per_session_state->value().groups,
        statement.payload.ix,
        grid_sender_index,
        getPayload(statement),
        true);
    return outcome::success();
  }

  void ParachainProcessorImpl::handle_incoming_acknowledgement(
      const libp2p::peer::PeerId &peer_id,
      const network::vstaging::BackedCandidateAcknowledgement
          &acknowledgement) {
    SL_TRACE(logger_,
             "`BackedCandidateAcknowledgement`. (candidate_hash={})",
             acknowledgement.candidate_hash);
    const auto &candidate_hash = acknowledgement.candidate_hash;
    SL_TRACE(logger_,
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
      SL_TRACE(logger_, "Handled statement from {} out of view", relay_parent);
      return;
    }
    auto &relay_parent_state = opt_parachain_state->get();
    BOOST_ASSERT(relay_parent_state.statement_store);

    SL_TRACE(logger_,
             "Handling incoming acknowledgement. (relay_parent={})",
             relay_parent);
    ManifestImportSuccessOpt x = handle_incoming_manifest_common(
        peer_id,
        candidate_hash,
        relay_parent,
        ManifestSummary{
            .claimed_parent_hash = parent_head_data_hash,
            .claimed_group_index = group_index,
            .statement_knowledge = acknowledgement.statement_knowledge,
        },
        para_id,
        grid::ManifestKind::Acknowledgement);
    if (!x) {
      return;
    }

    SL_TRACE(
        logger_, "Check local validator. (relay_parent = {})", relay_parent);
    if (!relay_parent_state.local_validator) {
      return;
    }

    const auto sender_index = x->sender_index;
    auto &local_validator = *relay_parent_state.local_validator;

    SL_TRACE(logger_, "Post ack. (relay_parent = {})", relay_parent);
    auto messages = post_acknowledgement_statement_messages(
        sender_index,
        relay_parent,
        local_validator.grid_tracker,
        *relay_parent_state.statement_store,
        relay_parent_state.per_session_state->value().groups,
        group_index,
        candidate_hash,
        peer_id,
        network::CollationVersion::VStaging);
    if (!messages.empty()) {
      auto se = pm_->getStreamEngine();
      SL_TRACE(logger_, "Sending messages. (relay_parent = {})", relay_parent);
      for (auto &msg : messages) {
        if (auto m =
                if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
          auto message = std::make_shared<network::WireMessage<
              network::vstaging::ValidatorProtocolMessage>>(
              std::move(m->get()));
          se->send(peer_id, router_->getValidationProtocolVStaging(), message);
        } else {
          assert(false);
        }
      }
    }
  }

  // Handles BackedCandidateManifest message
  //  It performs various checks and operations, and if everything is
  //  successful, it sends acknowledgement and statement messages to the
  //  validators group or sends a request to fetch the attested candidate.
  void ParachainProcessorImpl::handle_incoming_manifest(
      const libp2p::peer::PeerId &peer_id,
      const network::vstaging::BackedCandidateManifest &manifest) {
    SL_TRACE(logger_,
             "`BackedCandidateManifest`. (relay_parent={}, "
             "candidate_hash={}, para_id={}, parent_head_data_hash={})",
             manifest.relay_parent,
             manifest.candidate_hash,
             manifest.para_id,
             manifest.parent_head_data_hash);
    auto relay_parent_state = tryGetStateByRelayParent(manifest.relay_parent);
    if (!relay_parent_state) {
      SL_WARN(logger_,
              "After BackedCandidateManifest no parachain state on "
              "relay_parent. (relay "
              "parent={})",
              manifest.relay_parent);
      return;
    }

    if (!relay_parent_state->get().statement_store) {
      SL_ERROR(logger_,
               "Statement store is not initialized. (relay parent={})",
               manifest.relay_parent);
      return;
    }

    SL_TRACE(logger_,
             "Handling incoming manifest common. (relay_parent={}, "
             "candidate_hash={})",
             manifest.relay_parent,
             manifest.candidate_hash);
    ManifestImportSuccessOpt x = handle_incoming_manifest_common(
        peer_id,
        manifest.candidate_hash,
        manifest.relay_parent,
        ManifestSummary{
            .claimed_parent_hash = manifest.parent_head_data_hash,
            .claimed_group_index = manifest.group_index,
            .statement_knowledge = manifest.statement_knowledge,
        },
        manifest.para_id,
        grid::ManifestKind::Full);
    if (!x) {
      return;
    }

    const auto sender_index = x->sender_index;
    if (x->acknowledge) {
      SL_TRACE(logger_,
               "Known candidate - acknowledging manifest. (candidate hash={})",
               manifest.candidate_hash);

      SL_TRACE(logger_,
               "Get groups. (relay_parent={}, candidate_hash={})",
               manifest.relay_parent,
               manifest.candidate_hash);
      auto group =
          relay_parent_state->get().per_session_state->value().groups.get(
              manifest.group_index);
      if (!group) {
        return;
      }

      network::vstaging::StatementFilter local_knowledge =
          local_knowledge_filter(group->size(),
                                 manifest.group_index,
                                 manifest.candidate_hash,
                                 *relay_parent_state->get().statement_store);
      SL_TRACE(logger_,
               "Get ack and statement messages. (relay_parent={}, "
               "candidate_hash={})",
               manifest.relay_parent,
               manifest.candidate_hash);
      auto messages = acknowledgement_and_statement_messages(
          peer_id,
          network::CollationVersion::VStaging,
          sender_index,
          relay_parent_state->get().per_session_state->value().groups,
          relay_parent_state->get(),
          manifest.relay_parent,
          manifest.group_index,
          manifest.candidate_hash,
          local_knowledge);

      if (messages.empty()) {
        return;
      }

      SL_TRACE(logger_,
               "Send messages. (relay_parent={}, candidate_hash={})",
               manifest.relay_parent,
               manifest.candidate_hash);
      auto se = pm_->getStreamEngine();
      for (auto &[peers, msg] : messages) {
        if (auto m =
                if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
          auto message = std::make_shared<network::WireMessage<
              network::vstaging::ValidatorProtocolMessage>>(
              std::move(m->get()));
          for (const auto &p : peers) {
            se->send(p, router_->getValidationProtocolVStaging(), message);
          }
        } else {
          assert(false);
        }
      }
    } else if (!candidates_.is_confirmed(manifest.candidate_hash)) {
      SL_TRACE(
          logger_,
          "Request attested candidate. (relay_parent={}, candidate_hash={})",
          manifest.relay_parent,
          manifest.candidate_hash);
      request_attested_candidate(peer_id,
                                 relay_parent_state->get(),
                                 manifest.relay_parent,
                                 manifest.candidate_hash,
                                 manifest.group_index);
    }
  }

  outcome::result<
      std::reference_wrapper<const network::vstaging::SignedCompactStatement>>
  ParachainProcessorImpl::check_statement_signature(
      SessionIndex session_index,
      const std::vector<ValidatorId> &validators,
      const RelayHash &relay_parent,
      const network::vstaging::SignedCompactStatement &statement) {
    OUTCOME_TRY(signing_context,
                SigningContext::make(parachain_host_, relay_parent));
    OUTCOME_TRY(verified,
                crypto_provider_->verify(
                    statement.signature,
                    signing_context.signable(*hasher_, getPayload(statement)),
                    validators[statement.payload.ix]));

    if (!verified) {
      return Error::INCORRECT_SIGNATURE;
    }
    return std::cref(statement);
  }

  outcome::result<std::optional<network::vstaging::SignedCompactStatement>>
  ParachainProcessorImpl::handle_cluster_statement(
      const RelayHash &relay_parent,
      ClusterTracker &cluster_tracker,
      SessionIndex session,
      const runtime::SessionInfo &session_info,
      const network::vstaging::SignedCompactStatement &statement,
      ValidatorIndex cluster_sender_index) {
    const auto accept = cluster_tracker.can_receive(
        cluster_sender_index,
        statement.payload.ix,
        network::vstaging::from(getPayload(statement)));
    if (accept != outcome::success(Accept::Ok)
        && accept != outcome::success(Accept::WithPrejudice)) {
      SL_ERROR(logger_, "Reject outgoing error.");
      return Error::CLUSTER_TRACKER_ERROR;
    }
    OUTCOME_TRY(_,
                check_statement_signature(
                    session, session_info.validators, relay_parent, statement));

    cluster_tracker.note_received(
        cluster_sender_index,
        statement.payload.ix,
        network::vstaging::from(getPayload(statement)));

    const auto should_import = (outcome::success(Accept::Ok) == accept);
    if (should_import) {
      return statement;
    }
    return std::nullopt;
  }

  void ParachainProcessorImpl::handle_incoming_statement(
      const libp2p::peer::PeerId &peer_id,
      const network::vstaging::StatementDistributionMessageStatement &stm) {
    SL_TRACE(logger_,
             "`StatementDistributionMessageStatement`. (relay_parent={}, "
             "candidate_hash={})",
             stm.relay_parent,
             candidateHash(getPayload(stm.compact)));
    auto parachain_state = tryGetStateByRelayParent(stm.relay_parent);
    if (!parachain_state) {
      SL_TRACE(logger_,
               "After request pov no parachain state on relay_parent. (relay "
               "parent={})",
               stm.relay_parent);
      return;
    }

    const auto &session_info =
        parachain_state->get().per_session_state->value().session_info;
    if (!parachain_state->get().local_validator) {
      return;
    }

    auto &local_validator = *parachain_state->get().local_validator;
    auto originator_group =
        parachain_state->get()
            .per_session_state->value()
            .groups.byValidatorIndex(stm.compact.payload.ix);
    if (!originator_group) {
      SL_TRACE(logger_,
               "No correct validator index in statement. (relay parent={}, "
               "validator={})",
               stm.relay_parent,
               stm.compact.payload.ix);
      return;
    }

    /// TODO(iceseer): do `disabled validators`
    /// https://github.com/qdrvm/kagome/issues/2060

    auto &active = local_validator.active;
    auto cluster_sender_index = [&]() -> std::optional<ValidatorIndex> {
      std::span<const ValidatorIndex> allowed_senders;
      if (active) {
        allowed_senders = active->cluster_tracker.senders_for_originator(
            stm.compact.payload.ix);
      }

      if (auto peer = query_audi_->get(peer_id)) {
        for (const auto i : allowed_senders) {
          if (i < session_info.discovery_keys.size()
              && *peer == session_info.discovery_keys[i]) {
            return i;
          }
        }
      }
      return std::nullopt;
    }();

    if (active && cluster_sender_index) {
      if (handle_cluster_statement(
              stm.relay_parent,
              active->cluster_tracker,
              parachain_state->get().per_session_state->value().session,
              parachain_state->get().per_session_state->value().session_info,
              stm.compact,
              *cluster_sender_index)
              .has_error()) {
        return;
      }
    } else {
      std::optional<std::pair<ValidatorIndex, bool>> grid_sender_index;
      for (const auto &[i, validator_knows_statement] :
           local_validator.grid_tracker.direct_statement_providers(
               parachain_state->get().per_session_state->value().groups,
               stm.compact.payload.ix,
               getPayload(stm.compact))) {
        if (i >= session_info.discovery_keys.size()) {
          continue;
        }

        /// TODO(iceseer): do check is authority
        /// const auto &ad = opt_session_info->discovery_keys[i];
        grid_sender_index.emplace(i, validator_knows_statement);
        break;
      }

      if (!grid_sender_index) {
        return;
      }

      const auto &[gsi, validator_knows_statement] = *grid_sender_index;
      if (!validator_knows_statement) {
        if (handle_grid_statement(stm.relay_parent,
                                  parachain_state->get(),
                                  local_validator.grid_tracker,
                                  stm.compact,
                                  gsi)
                .has_error()) {
          return;
        }
      } else {
        return;
      }
    }

    const auto &statement = getPayload(stm.compact);
    const auto originator_index = stm.compact.payload.ix;
    const auto &candidate_hash = candidateHash(getPayload(stm.compact));
    const bool res = candidates_.insert_unconfirmed(peer_id,
                                                    candidate_hash,
                                                    stm.relay_parent,
                                                    *originator_group,
                                                    std::nullopt);
    if (!res) {
      return;
    }

    const auto confirmed = candidates_.get_confirmed(candidate_hash);
    const auto is_confirmed = candidates_.is_confirmed(candidate_hash);
    const auto &group = session_info.validator_groups[*originator_group];

    if (!is_confirmed) {
      request_attested_candidate(peer_id,
                                 parachain_state->get(),
                                 stm.relay_parent,
                                 candidate_hash,
                                 *originator_group);
    }

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// check statement signature

    const auto was_fresh_opt = parachain_state->get().statement_store->insert(
        parachain_state->get().per_session_state->value().groups,
        stm.compact,
        StatementOrigin::Remote);
    if (!was_fresh_opt) {
      SL_WARN(logger_,
              "Accepted message from unknown validator. (relay parent={}, "
              "validator={})",
              stm.relay_parent,
              stm.compact.payload.ix);
      return;
    }

    if (!*was_fresh_opt) {
      SL_TRACE(logger_,
               "Statement was not fresh. (relay parent={}, validator={})",
               stm.relay_parent,
               stm.compact.payload.ix);
      return;
    }

    const auto is_importable = candidates_.is_importable(candidate_hash);
    if (parachain_state->get().per_session_state->value().grid_view) {
      local_validator.grid_tracker.learned_fresh_statement(
          parachain_state->get().per_session_state->value().groups,
          *parachain_state->get().per_session_state->value().grid_view,
          originator_index,
          statement);
    }

    if (is_importable && confirmed) {
      send_backing_fresh_statements(confirmed->get(),
                                    stm.relay_parent,
                                    parachain_state->get(),
                                    group,
                                    candidate_hash);
    }

    circulate_statement(stm.relay_parent, parachain_state->get(), stm.compact);
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
      handle_incoming_acknowledgement(peer_id, inner->get());
    } else if (auto manifest =
                   if_type<const network::vstaging::BackedCandidateManifest>(
                       msg)) {
      handle_incoming_manifest(peer_id, manifest->get());
    } else if (auto stm =
                   if_type<const network::vstaging::
                               StatementDistributionMessageStatement>(msg)) {
      handle_incoming_statement(peer_id, stm->get());
    } else {
      SL_ERROR(logger_, "Skipped message.");
    }
  }

  void ParachainProcessorImpl::circulate_statement(
      const RelayHash &relay_parent,
      RelayParentState &relay_parent_state,
      const IndexedAndSigned<network::vstaging::CompactStatement> &statement) {
    const auto &session_info =
        relay_parent_state.per_session_state->value().session_info;
    const auto &compact_statement = getPayload(statement);
    const auto &candidate_hash = candidateHash(compact_statement);
    const auto originator = statement.payload.ix;
    const auto is_confirmed = candidates_.is_confirmed(candidate_hash);

    if (!relay_parent_state.local_validator) {
      return;
    }

    enum DirectTargetKind {
      Cluster,
      Grid,
    };

    auto &local_validator = *relay_parent_state.local_validator;
    auto targets =
        [&]() -> std::vector<std::pair<ValidatorIndex, DirectTargetKind>> {
      auto statement_group =
          relay_parent_state.per_session_state->value().groups.byValidatorIndex(
              originator);

      bool cluster_relevant = false;
      std::vector<std::pair<ValidatorIndex, DirectTargetKind>> targets;
      std::span<const ValidatorIndex> all_cluster_targets;

      if (local_validator.active) {
        auto &active = *local_validator.active;
        cluster_relevant =
            (statement_group && *statement_group == active.group);
        if (is_confirmed && cluster_relevant) {
          for (const auto v : active.cluster_tracker.targets()) {
            if (active.cluster_tracker
                    .can_send(v,
                              originator,
                              network::vstaging::from(compact_statement))
                    .has_error()) {
              continue;
            }
            if (v == active.index) {
              continue;
            }
            if (v >= session_info.discovery_keys.size()) {
              continue;
            }
            targets.emplace_back(v, DirectTargetKind::Cluster);
          }
        }
        all_cluster_targets = active.cluster_tracker.targets();
      }

      for (const auto v : local_validator.grid_tracker.direct_statement_targets(
               relay_parent_state.per_session_state->value().groups,
               originator,
               compact_statement)) {
        const auto can_use_grid =
            !cluster_relevant
            || std::find(
                   all_cluster_targets.begin(), all_cluster_targets.end(), v)
                   == all_cluster_targets.end();
        if (!can_use_grid) {
          continue;
        }
        if (v >= session_info.discovery_keys.size()) {
          continue;
        }
        targets.emplace_back(v, DirectTargetKind::Grid);
      }

      return targets;
    }();

    std::vector<std::pair<libp2p::peer::PeerId, network::CollationVersion>>
        statement_to_peers;
    for (const auto &[target, kind] : targets) {
      auto peer = query_audi_->get(session_info.discovery_keys[target]);
      if (!peer) {
        continue;
      }

      auto peer_state = pm_->getPeerState(peer->id);
      if (!peer_state) {
        continue;
      }

      if (!peer_state->get().knows_relay_parent(relay_parent)) {
        continue;
      }

      network::CollationVersion version = network::CollationVersion::VStaging;
      if (peer_state->get().version) {
        version = *peer_state->get().version;
      }

      switch (kind) {
        case Cluster: {
          auto &active = *local_validator.active;
          if (active.cluster_tracker
                  .can_send(target,
                            originator,
                            network::vstaging::from(compact_statement))
                  .has_value()) {
            active.cluster_tracker.note_sent(
                target, originator, network::vstaging::from(compact_statement));
            statement_to_peers.emplace_back(peer->id, version);
          }
        } break;
        case Grid: {
          statement_to_peers.emplace_back(peer->id, version);
          local_validator.grid_tracker.sent_or_received_direct_statement(
              relay_parent_state.per_session_state->value().groups,
              originator,
              target,
              compact_statement,
              false);
        } break;
      }
    }

    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    auto message_v2 = std::make_shared<
        network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
        kagome::network::vstaging::ValidatorProtocolMessage{
            kagome::network::vstaging::StatementDistributionMessage{
                kagome::network::vstaging::
                    StatementDistributionMessageStatement{
                        .relay_parent = relay_parent,
                        .compact = statement,
                    }}});
    SL_TRACE(
        logger_,
        "Send statements to validators. (relay_parent={}, validators_count={})",
        relay_parent,
        statement_to_peers.size());
    for (const auto &[peer, version] : statement_to_peers) {
      if (version == network::CollationVersion::VStaging) {
        se->send(peer, router_->getValidationProtocolVStaging(), message_v2);
      } else {
        BOOST_ASSERT(false);
      }
    }
  }

  void ParachainProcessorImpl::request_attested_candidate(
      const libp2p::peer::PeerId &peer,
      RelayParentState &relay_parent_state,
      const RelayHash &relay_parent,
      const CandidateHash &candidate_hash,
      GroupIndex group_index) {
    if (!relay_parent_state.local_validator) {
      return;
    }
    auto &local_validator = *relay_parent_state.local_validator;

    const auto &session_info =
        relay_parent_state.per_session_state->value().session_info;
    auto group =
        relay_parent_state.per_session_state->value().groups.get(group_index);
    if (!group) {
      return;
    }
    const auto seconding_limit =
        relay_parent_state.prospective_parachains_mode->max_candidate_depth + 1;

    SL_TRACE(logger_,
             "Form unwanted mask. (relay_parent={}, candidate_hash={})",
             relay_parent,
             candidate_hash);
    network::vstaging::StatementFilter unwanted_mask(group->size());
    for (size_t i = 0; i < group->size(); ++i) {
      const auto v = (*group)[i];
      if (relay_parent_state.statement_store->seconded_count(v)
          >= seconding_limit) {
        unwanted_mask.seconded_in_group.bits[i] = true;
      }
    }

    /// TODO(iceseer): do `disabled validators`
    /// Add disabled validators to the unwanted mask.
    /// https://github.com/qdrvm/kagome/issues/2060

    auto backing_threshold = [&]() -> std::optional<size_t> {
      auto bt = relay_parent_state.per_session_state->value()
                    .groups.get_size_and_backing_threshold(group_index);
      return bt ? std::get<1>(*bt) : std::optional<size_t>{};
    }();

    SL_TRACE(logger_,
             "Enumerate peers. (relay_parent={}, candidate_hash={})",
             relay_parent,
             candidate_hash);
    std::optional<network::vstaging::StatementFilter> target;
    auto audi = query_audi_->get(peer);
    if (!audi) {
      SL_TRACE(logger_,
               "No audi. (relay_parent={}, candidate_hash={})",
               relay_parent,
               candidate_hash);
      return;
    }

    ValidatorIndex validator_id = 0;
    for (; validator_id < session_info.discovery_keys.size(); ++validator_id) {
      if (session_info.discovery_keys[validator_id] == *audi) {
        SL_TRACE(logger_,
                 "Captured validator. (relay_parent={}, candidate_hash={})",
                 relay_parent,
                 candidate_hash);
        break;
      }
    }

    if (validator_id >= session_info.discovery_keys.size()) {
      return;
    }

    auto filter = [&]() -> std::optional<network::vstaging::StatementFilter> {
      if (local_validator.active) {
        if (local_validator.active->cluster_tracker.knows_candidate(
                validator_id, candidate_hash)) {
          return network::vstaging::StatementFilter(
              local_validator.active->cluster_tracker.targets().size());
        }
      }

      auto filter = local_validator.grid_tracker.advertised_statements(
          validator_id, candidate_hash);
      if (filter) {
        return filter;
      }

      SL_TRACE(logger_,
               "No filter. (relay_parent={}, candidate_hash={})",
               relay_parent,
               candidate_hash);
      return std::nullopt;
    }();

    if (!filter) {
      return;
    }

    filter->mask_seconded(unwanted_mask.seconded_in_group);
    filter->mask_valid(unwanted_mask.validated_in_group);

    if (!backing_threshold
        || (filter->has_seconded()
            && filter->backing_validators() >= *backing_threshold)) {
      network::vstaging::StatementFilter f(group->size());
      target.emplace(std::move(f));
    } else {
      SL_TRACE(
          logger_,
          "Not pass backing threshold. (relay_parent={}, candidate_hash={})",
          relay_parent,
          candidate_hash);
      return;
    }

    if (!target) {
      SL_TRACE(logger_,
               "Target not found. (relay_parent={}, candidate_hash={})",
               relay_parent,
               candidate_hash);
      return;
    }

    const auto &um = *target;
    SL_TRACE(logger_,
             "Requesting. (peer={}, relay_parent={}, candidate_hash={})",
             peer,
             relay_parent,
             candidate_hash);
    router_->getFetchAttestedCandidateProtocol()->doRequest(
        peer,
        network::vstaging::AttestedCandidateRequest{
            .candidate_hash = candidate_hash,
            .mask = um,
        },
        [wptr{weak_from_this()},
         relay_parent{relay_parent},
         candidate_hash{candidate_hash},
         group_index{group_index}](
            outcome::result<network::vstaging::AttestedCandidateResponse>
                r) mutable {
          if (auto self = wptr.lock()) {
            self->handleFetchedStatementResponse(
                std::move(r), relay_parent, candidate_hash, group_index);
          }
        });
  }

  void ParachainProcessorImpl::handleFetchedStatementResponse(
      outcome::result<network::vstaging::AttestedCandidateResponse> &&r,
      const RelayHash &relay_parent,
      const CandidateHash &candidate_hash,
      GroupIndex group_index) {
    REINVOKE(*main_pool_handler_,
             handleFetchedStatementResponse,
             std::move(r),
             relay_parent,
             candidate_hash,
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

    SL_INFO(logger_,
            "Fetch attested candidate success. (relay parent={}, "
            "candidate={}, group index={}, statements={})",
            relay_parent,
            candidate_hash,
            group_index,
            response.statements.size());
    for (const auto &statement : response.statements) {
      parachain_state->get().statement_store->insert(
          parachain_state->get().per_session_state->value().groups,
          statement,
          StatementOrigin::Remote);
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
      SL_INFO(logger_,
              "Not importable. (relay parent={}, "
              "candidate={}, group index={})",
              relay_parent,
              candidate_hash,
              group_index);
      return;
    }

    const auto &groups =
        parachain_state->get().per_session_state->value().groups;
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

    SL_INFO(logger_,
            "Send fresh statements. (relay parent={}, "
            "candidate={})",
            relay_parent,
            candidate_hash);
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

          const auto &session_info =
              prs->get().per_session_state->value().session_info;
          if (!group_index
              || *group_index >= session_info.validator_groups.size()) {
            return;
          }

          const auto &group = session_info.validator_groups[*group_index];
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

  /// TODO(iceseer): do remove
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

  void ParachainProcessorImpl::send_cluster_candidate_statements(
      const CandidateHash &candidate_hash, const RelayHash &relay_parent) {
    auto relay_parent_state = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state) {
      return;
    }

    auto local_group = relay_parent_state->get().our_group;
    if (!local_group) {
      return;
    }

    auto group =
        relay_parent_state->get().per_session_state->value().groups.get(
            *local_group);
    if (!group) {
      return;
    }
    auto group_size = group->size();

    relay_parent_state->get().statement_store->groupStatements(
        *group,
        candidate_hash,
        network::vstaging::StatementFilter(group_size, true),
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) {
          circulate_statement(
              relay_parent, relay_parent_state->get(), statement);
        });
  }

  void ParachainProcessorImpl::apply_post_confirmation(
      const PostConfirmation &post_confirmation) {
    const auto candidate_hash = candidateHash(post_confirmation.hypothetical);
    send_cluster_candidate_statements(
        candidate_hash, relayParent(post_confirmation.hypothetical));

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

          SignedFullStatementWithPVD carrying_pvd{
              .payload =
                  {
                      .payload = visit_in_place(
                          compact.inner_value,
                          [&](const network::vstaging::SecondedCandidateHash &)
                              -> StatementWithPVD {
                            return StatementWithPVDSeconded{
                                .committed_receipt = confirmed.receipt,
                                .pvd = confirmed.persisted_validation_data,
                            };
                          },
                          [](const network::vstaging::ValidCandidateHash &val)
                              -> StatementWithPVD {
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
          };

          main_pool_handler_->execute(
              [wself{weak_from_this()},
               relay_parent{relay_parent},
               carrying_pvd{std::move(carrying_pvd)}]() {
                if (auto self = wself.lock()) {
                  SL_TRACE(self->logger_, "Handle statement {}", relay_parent);
                  self->handleStatement(relay_parent, carrying_pvd);
                }
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
        auto res_pvd = fetchPersistedValidationData(
            statement_msg->relay_parent, ccr->get().descriptor.para_id);
        if (res_pvd.has_error()) {
          SL_TRACE(logger_, "No pvd fetched. (error={})", res_pvd.error());
          return;
        }
        std::optional<runtime::PersistedValidationData> pvd =
            std::move(*res_pvd.value());
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
        },
        [&](const auto &m) { SL_WARN(logger_, "UNSUPPORTED Version"); });
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

  void ParachainProcessorImpl::kickOffValidationWork(
      const RelayHash &relay_parent,
      AttestingData &attesting_data,
      const runtime::PersistedValidationData &persisted_validation_data,
      RelayParentState &parachain_state) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    const auto candidate_hash{attesting_data.candidate.hash(*hasher_)};
    if (parachain_state.issued_statements.contains(candidate_hash)) {
      return;
    }

    const auto &session_info =
        parachain_state.per_session_state->value().session_info;
    if (session_info.discovery_keys.size() <= attesting_data.from_validator) {
      SL_ERROR(logger_,
               "Invalid validator index.(relay_parent={}, validator_index={})",
               relay_parent,
               attesting_data.from_validator);
      return;
    }

    const auto &authority_id =
        session_info.discovery_keys[attesting_data.from_validator];
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
                                    pov_response_result.error());
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
                  relay_parent);
            }
          });
    }
  }

  outcome::result<network::vstaging::AttestedCandidateResponse>
  ParachainProcessorImpl::OnFetchAttestedCandidateRequest(
      const network::vstaging::AttestedCandidateRequest &request,
      const libp2p::peer::PeerId &peer_id) {
    auto confirmed = candidates_.get_confirmed(request.candidate_hash);
    if (!confirmed) {
      return Error::NOT_CONFIRMED;
    }

    auto relay_parent_state =
        tryGetStateByRelayParent(confirmed->get().relay_parent());
    if (!relay_parent_state) {
      return Error::NO_STATE;
    }

    auto &local_validator = relay_parent_state->get().local_validator;
    if (!local_validator) {
      return Error::NOT_A_VALIDATOR;
    }
    BOOST_ASSERT(relay_parent_state->get().statement_store);
    BOOST_ASSERT(relay_parent_state->get().our_index);

    const auto &session_info =
        relay_parent_state->get().per_session_state->value().session_info;
    const auto &groups =
        relay_parent_state->get().per_session_state->value().groups;
    auto group = groups.get(confirmed->get().group_index());
    if (!group) {
      SL_ERROR(logger_,
               "Unexpected array bound for groups. (relay parent={})",
               confirmed->get().relay_parent());
      return Error::OUT_OF_BOUND;
    }

    const auto group_size = group->size();
    auto &mask = request.mask;
    if (mask.seconded_in_group.bits.size() != group_size
        || mask.validated_in_group.bits.size() != group_size) {
      return Error::INCORRECT_BITFIELD_SIZE;
    }

    auto &active = local_validator->active;
    auto [validator_id, is_cluster] =
        [&]() -> std::pair<std::optional<ValidatorIndex>, bool> {
      std::optional<ValidatorIndex> validator_id;
      bool is_cluster = false;

      do {
        auto audi = query_audi_->get(peer_id);
        if (!audi) {
          SL_TRACE(logger_, "No audi. (peer={})", peer_id);
          break;
        }

        ValidatorIndex v = 0;
        for (; v < session_info.discovery_keys.size(); ++v) {
          if (session_info.discovery_keys[v] == *audi) {
            SL_TRACE(logger_,
                     "Captured validator. (relay_parent={}, candidate_hash={})",
                     confirmed->get().relay_parent(),
                     request.candidate_hash);
            break;
          }
        }

        if (v >= session_info.discovery_keys.size()) {
          break;
        }

        if (active
            && active->cluster_tracker.can_request(v, request.candidate_hash)) {
          validator_id = v;
          is_cluster = true;
          break;
        }

        if (local_validator->grid_tracker.can_request(v,
                                                      request.candidate_hash)) {
          validator_id = v;
          break;
        }
      } while (false);

      return {validator_id, is_cluster};
    }();

    if (!validator_id) {
      return Error::OUT_OF_BOUND;
    }

    auto init_with_not = [](scale::BitVec &dst, const scale::BitVec &src) {
      dst.bits.reserve(src.bits.size());
      for (const auto i : src.bits) {
        dst.bits.emplace_back(!i);
      }
    };

    network::vstaging::StatementFilter and_mask;
    init_with_not(and_mask.seconded_in_group, request.mask.seconded_in_group);
    init_with_not(and_mask.validated_in_group, request.mask.validated_in_group);

    /// TODO(iceseer): do `disabled validators` check
    /// https://github.com/qdrvm/kagome/issues/2060
    std::vector<IndexedAndSigned<network::vstaging::CompactStatement>>
        statements;
    relay_parent_state->get().statement_store->groupStatements(
        *group,
        request.candidate_hash,
        and_mask,
        [&](const IndexedAndSigned<network::vstaging::CompactStatement>
                &statement) { statements.emplace_back(statement); });

    for (const auto &statement : statements) {
      if (is_cluster) {
        active->cluster_tracker.note_sent(
            *validator_id,
            statement.payload.ix,
            network::vstaging::from(getPayload(statement)));
      } else {
        local_validator->grid_tracker.sent_or_received_direct_statement(
            groups,
            statement.payload.ix,
            *validator_id,
            getPayload(statement),
            false);
      }
    }

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

  outcome::result<
      std::reference_wrapper<ParachainProcessorImpl::RelayParentState>>
  ParachainProcessorImpl::getStateByRelayParent(
      const primitives::BlockHash &relay_parent) {
    if (auto per_relay_parent = tryGetStateByRelayParent(relay_parent)) {
      return *per_relay_parent;
    }
    return Error::OUT_OF_VIEW;
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
    const auto &assigned_para = parachain_state.assigned_para;
    const auto &assigned_core = parachain_state.assigned_core;
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
      if (!assigned_core || result->group_id != *assigned_core) {
        SL_TRACE(logger_,
                 "Registered statement from not our group(assigned_para "
                 "our={}, assigned_core our={}, registered={}).",
                 assigned_para,
                 assigned_core,
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
                    .candidate =
                        opt_candidate->get().candidate.to_plain(*hasher_),
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
      GroupIndex group_id,
      const primitives::BlockHash &candidate_hash,
      const network::SignedStatement &statement) {
    SL_TRACE(
        logger_, "Import statement into table.(candidate={})", candidate_hash);
    return backing_store_->put(
        relay_parent,
        group_id,
        relayParentState.table_context.groups,
        statement,
        relayParentState.prospective_parachains_mode.has_value());
  }

  void ParachainProcessorImpl::provide_candidate_to_grid(
      const CandidateHash &candidate_hash,
      RelayParentState &relay_parent_state,
      const ConfirmedCandidate &confirmed_candidate,
      const runtime::SessionInfo &session_info) {
    if (!relay_parent_state.local_validator) {
      return;
    }
    auto &local_validator = *relay_parent_state.local_validator;

    const auto relay_parent = confirmed_candidate.relay_parent();
    const auto group_index = confirmed_candidate.group_index();

    if (!relay_parent_state.per_session_state->value().grid_view) {
      SL_TRACE(logger_,
               "Cannot handle backable candidate due to lack of topology. "
               "(candidate={}, relay_parent={})",
               candidate_hash,
               relay_parent);
      return;
    }

    const auto &grid_view =
        *relay_parent_state.per_session_state->value().grid_view;
    const auto group =
        relay_parent_state.per_session_state->value().groups.get(group_index);
    if (!group) {
      SL_TRACE(logger_,
               "Handled backed candidate with unknown group? (candidate={}, "
               "relay_parent={}, group_index={})",
               candidate_hash,
               relay_parent,
               group_index);
      return;
    }
    const auto group_size = group->size();

    SL_TRACE(logger_,
             "======================== GRID VIEW group={} relay_parent={} "
             "our_index={} our_id={} our_dk={} in_per_session={} "
             "========================",
             group_index,
             relay_parent,
             *relay_parent_state.our_index,
             session_info.validators[*relay_parent_state.our_index],
             session_info.discovery_keys[*relay_parent_state.our_index],
             *relay_parent_state.per_session_state->value().our_index);

    for (size_t k = 0; k < grid_view.size(); ++k) {
      const auto &v = grid_view[k];
      SL_TRACE(logger_, "\tGroup {}", k);
      for (const auto vi : v.sending) {
        SL_TRACE(logger_, "\t\tS: {}", vi);
      }
      for (const auto vi : v.receiving) {
        SL_TRACE(logger_, "\t\tR: {}", vi);
      }
    }

    SL_TRACE(
        logger_,
        "=================================================================");

    auto filter = local_knowledge_filter(group_size,
                                         group_index,
                                         candidate_hash,
                                         *relay_parent_state.statement_store);

    auto actions = local_validator.grid_tracker.add_backed_candidate(
        grid_view, candidate_hash, group_index, filter);

    network::vstaging::BackedCandidateManifest manifest{
        .relay_parent = relay_parent,
        .candidate_hash = candidate_hash,
        .group_index = group_index,
        .para_id = confirmed_candidate.para_id(),
        .parent_head_data_hash = confirmed_candidate.parent_head_data_hash(),
        .statement_knowledge = filter};

    network::vstaging::BackedCandidateAcknowledgement acknowledgement{
        .candidate_hash = candidate_hash, .statement_knowledge = filter};

    std::vector<std::pair<libp2p::peer::PeerId, network::CollationVersion>>
        manifest_peers;
    std::vector<std::pair<libp2p::peer::PeerId, network::CollationVersion>>
        ack_peers;
    std::deque<std::pair<std::vector<libp2p::peer::PeerId>,
                         network::VersionedValidatorProtocolMessage>>
        post_statements;

    for (const auto &[v, action] : actions) {
      auto peer_opt = query_audi_->get(session_info.discovery_keys[v]);
      if (!peer_opt) {
        SL_TRACE(logger_,
                 "No peer info. (relay_parent={}, validator_index={}, "
                 "candidate_hash={})",
                 relay_parent,
                 v,
                 candidate_hash);
        continue;
      }

      auto peer_state = pm_->getPeerState(peer_opt->id);
      if (!peer_state) {
        SL_TRACE(logger_,
                 "No peer state. (relay_parent={}, peer={}, candidate_hash={})",
                 relay_parent,
                 peer_opt->id,
                 candidate_hash);
        continue;
      }

      if (!peer_state->get().knows_relay_parent(relay_parent)) {
        SL_TRACE(logger_,
                 "Peer doesn't know relay parent. (relay_parent={}, peer={}, "
                 "candidate_hash={})",
                 relay_parent,
                 peer_opt->id,
                 candidate_hash);
        continue;
      }

      switch (action) {
        case grid::ManifestKind::Full: {
          SL_TRACE(logger_, "Full manifest -> {}", v);
          manifest_peers.emplace_back(peer_opt->id,
                                      network::CollationVersion::VStaging);
        } break;
        case grid::ManifestKind::Acknowledgement: {
          SL_TRACE(logger_, "Ack manifest -> {}", v);
          ack_peers.emplace_back(peer_opt->id,
                                 network::CollationVersion::VStaging);
        } break;
      }

      local_validator.grid_tracker.manifest_sent_to(
          relay_parent_state.per_session_state->value().groups,
          v,
          candidate_hash,
          filter);

      auto msgs = post_acknowledgement_statement_messages(
          v,
          relay_parent,
          local_validator.grid_tracker,
          *relay_parent_state.statement_store,
          relay_parent_state.per_session_state->value().groups,
          group_index,
          candidate_hash,
          peer_opt->id,
          network::CollationVersion::VStaging);

      for (auto &msg : msgs) {
        post_statements.emplace_back(
            std::vector<libp2p::peer::PeerId>{peer_opt->id}, std::move(msg));
      }
    }

    auto se = pm_->getStreamEngine();
    BOOST_ASSERT(se);

    if (!manifest_peers.empty()) {
      SL_TRACE(logger_,
               "Sending manifest to v2 peers. (candidate_hash={}, "
               "local_validator={}, n_peers={})",
               candidate_hash,
               *relay_parent_state.our_index,
               manifest_peers.size());
      auto message = std::make_shared<
          network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
          kagome::network::vstaging::ValidatorProtocolMessage{
              kagome::network::vstaging::StatementDistributionMessage{
                  manifest}});
      for (const auto &[p, _] : manifest_peers) {
        se->send(p, router_->getValidationProtocolVStaging(), message);
      }
    }

    if (!ack_peers.empty()) {
      SL_TRACE(logger_,
               "Sending acknowledgement to v2 peers. (candidate_hash={}, "
               "local_validator={}, n_peers={})",
               candidate_hash,
               *relay_parent_state.our_index,
               ack_peers.size());
      auto message = std::make_shared<
          network::WireMessage<network::vstaging::ValidatorProtocolMessage>>(
          kagome::network::vstaging::ValidatorProtocolMessage{
              kagome::network::vstaging::StatementDistributionMessage{
                  acknowledgement}});
      for (const auto &[p, _] : ack_peers) {
        se->send(p, router_->getValidationProtocolVStaging(), message);
      }
    }

    if (!post_statements.empty()) {
      SL_TRACE(logger_,
               "Sending statements to v2 peers. (candidate_hash={}, "
               "local_validator={}, n_peers={})",
               candidate_hash,
               *relay_parent_state.our_index,
               post_statements.size());

      for (auto &[peers, msg] : post_statements) {
        if (auto m =
                if_type<network::vstaging::ValidatorProtocolMessage>(msg)) {
          auto message = std::make_shared<network::WireMessage<
              network::vstaging::ValidatorProtocolMessage>>(
              std::move(m->get()));
          for (const auto &p : peers) {
            se->send(p, router_->getValidationProtocolVStaging(), message);
          }
        } else {
          assert(false);
        }
      }
    }
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

    const auto &session_info =
        relay_parent_state_opt->get().per_session_state->value().session_info;
    provide_candidate_to_grid(
        candidate_hash, relay_parent_state_opt->get(), confirmed, session_info);

    prospective_backed_notification_fragment_tree_updates(
        confirmed.para_id(), confirmed.para_head());
  }

  outcome::result<BlockNumber>
  ParachainProcessorImpl::get_block_number_under_construction(
      const RelayHash &relay_parent) const {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    auto res_header = block_tree_->getBlockHeader(relay_parent);
    if (res_header.has_error()) {
      if (res_header.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
        return 0;
      }
      return res_header.error();
    }
    return res_header.value().number + 1;
  }

  bool ParachainProcessorImpl::bitfields_indicate_availability(
      size_t core_idx,
      const std::vector<BitfieldStore::SignedBitfield> &bitfields,
      const scale::BitVec &availability_) {
    scale::BitVec availability{availability_};
    const auto availability_len = availability.bits.size();

    for (const auto &bitfield : bitfields) {
      const auto validator_idx{size_t(bitfield.payload.ix)};
      if (validator_idx >= availability.bits.size()) {
        SL_WARN(logger_,
                "attempted to set a transverse bit at idx which is greater "
                "than bitfield size. (validator_idx={}, availability_len={})",
                validator_idx,
                availability_len);

        return false;
      }

      availability.bits[validator_idx] =
          availability.bits[validator_idx]
          || bitfield.payload.payload.bits[core_idx];
    }

    return 3 * approval::count_ones(availability)
        >= 2 * availability.bits.size();
  }

  std::vector<network::BackedCandidate>
  ParachainProcessorImpl::getBackedCandidates(const RelayHash &relay_parent) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    SL_TRACE(logger_, "Get backed candidates. (relay_parent={})", relay_parent);

    auto relay_parent_state_opt = tryGetStateByRelayParent(relay_parent);
    if (!relay_parent_state_opt) {
      return {};
    }

    if (!relay_parent_state_opt->get().prospective_parachains_mode) {
      return backing_store_->get(relay_parent);
    }

    BlockNumber block_number;
    if (auto r = get_block_number_under_construction(relay_parent);
        r.has_error()) {
      return {};
    } else {
      block_number = r.value();
    }

    using Ancestors = std::unordered_set<CandidateHash>;
    const auto &availability_cores =
        relay_parent_state_opt->get().availability_cores;

    std::map<ParachainId, size_t> scheduled_cores_per_para;
    std::unordered_map<ParachainId, Ancestors> ancestors;
    ancestors.reserve(availability_cores.size());

    const auto elastic_scaling_mvp =
        relay_parent_state_opt->get().inject_core_index;
    const auto bitfields = bitfield_store_->getBitfields(relay_parent);
    const auto cores_len =
        relay_parent_state_opt->get().availability_cores.size();

    for (size_t core_idx = 0; core_idx < cores_len; ++core_idx) {
      const runtime::CoreState &core =
          relay_parent_state_opt->get().availability_cores[core_idx];
      visit_in_place(
          core,
          [&](const network::ScheduledCore &scheduled_core) {
            scheduled_cores_per_para[scheduled_core.para_id] += 1;
          },
          [&](const runtime::OccupiedCore &occupied_core) {
            const bool is_available = bitfields_indicate_availability(
                core_idx, bitfields, occupied_core.availability);
            if (is_available) {
              ancestors[occupied_core.candidate_descriptor.para_id].insert(
                  occupied_core.candidate_hash);
              if (occupied_core.next_up_on_available) {
                scheduled_cores_per_para[occupied_core.next_up_on_available
                                             ->para_id] += 1;
              }
            } else if (occupied_core.time_out_at <= block_number) {
              if (occupied_core.next_up_on_time_out) {
                scheduled_cores_per_para[occupied_core.next_up_on_time_out
                                             ->para_id] += 1;
              }
            } else {
              ancestors[occupied_core.candidate_descriptor.para_id].insert(
                  occupied_core.candidate_hash);
            }
          },
          [&](const runtime::FreeCore &) {});
    }

    std::unordered_map<ParachainId, std::vector<std::pair<CandidateHash, Hash>>>
        selected_candidates;
    selected_candidates.reserve(scheduled_cores_per_para.size());

    auto ancestor_remove = [&](ParachainId para_id) -> Ancestors {
      auto it = ancestors.find(para_id);
      if (it == ancestors.end()) {
        return {};
      }

      auto result{std::move(it->second)};
      ancestors.erase(it);
      return result;
    };

    for (const auto &[para_id, core_count] : scheduled_cores_per_para) {
      auto para_ancestors = ancestor_remove(para_id);
      if (!elastic_scaling_mvp && core_count > 1) {
        continue;
      }

      std::vector<CandidateHash> para_ancestors_vec(
          std::move_iterator(para_ancestors.begin()),
          std::move_iterator(para_ancestors.end()));
      auto response = prospective_parachains_->answerGetBackableCandidates(
          relay_parent, para_id, core_count, para_ancestors_vec);

      if (response.empty()) {
        SL_TRACE(logger_,
                 "No backable candidate returned by prospective parachains. "
                 "(relay_parent={}, para_id={})",
                 relay_parent,
                 para_id);
        continue;
      }

      selected_candidates.emplace(para_id, std::move(response));
    }
    SL_TRACE(logger_,
             "Got backable candidates. (count={})",
             selected_candidates.size());

    std::unordered_map<ParachainId, std::vector<BackingStore::BackedCandidate>>
        backed;
    backed.reserve(selected_candidates.size());

    for (const auto &[para_id, para_candidates] : selected_candidates) {
      for (const auto &[c_hash, r_hash] : para_candidates) {
        auto rp_state = tryGetStateByRelayParent(r_hash);
        if (!rp_state) {
          SL_TRACE(logger_,
                   "Requested candidate's relay parent is out of view. "
                   "(relay_parent={}, r_hash={}, c_hash={})",
                   relay_parent,
                   r_hash,
                   c_hash);
          break;
        }

        if (auto attested =
                attested_candidate(r_hash,
                                   c_hash,
                                   rp_state->get().table_context,
                                   rp_state->get().minimum_backing_votes)) {
          if (auto b =
                  table_attested_to_backed(std::move(*attested),
                                           rp_state->get().table_context,
                                           rp_state->get().inject_core_index)) {
            backed[para_id].emplace_back(std::move(*b));
          } else {
            SL_TRACE(logger_,
                     "Candidate not attested -> backed. "
                     "(relay_parent={}, r_state={}, c_hash={})",
                     relay_parent,
                     r_hash,
                     c_hash);
          }
        } else {
          SL_TRACE(logger_,
                   "Candidate not attested. "
                   "(relay_parent={}, r_state={}, c_hash={})",
                   relay_parent,
                   r_hash,
                   c_hash);
        }
      }
    }

    SL_TRACE(logger_,
             "Got backed candidates. (relay_parent={}, backed_len={})",
             relay_parent,
             backed.size());
    bool with_validation_code = false;
    std::vector<BackingStore::BackedCandidate> merged_candidates;
    merged_candidates.reserve(availability_cores.size());

    for (const auto &[_, para_candidates] : backed) {
      for (const auto &candidate : para_candidates) {
        if (candidate.candidate.commitments.opt_para_runtime) {
          if (with_validation_code) {
            break;
          } else {
            with_validation_code = true;
          }
        }

        merged_candidates.emplace_back(candidate);
      }
    }

    SL_TRACE(logger_,
             "Selected backed candidates. (n_candidates={}, n_cores={}, "
             "relay_parent={})",
             merged_candidates.size(),
             availability_cores.size(),
             relay_parent);

    return merged_candidates;
  }

  std::optional<ParachainProcessorImpl::AttestedCandidate>
  ParachainProcessorImpl::attested(
      const network::CommittedCandidateReceipt &candidate,
      const BackingStore::StatementInfo &data,
      size_t validity_threshold) {
    const auto &validity_votes = data.validity_votes;
    const auto valid_votes = validity_votes.size();
    if (valid_votes < validity_threshold) {
      SL_TRACE(logger_,
               "Under threshold. (valid_votes={}, validity_threshold={})",
               valid_votes,
               validity_threshold);
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
      } else {
        SL_TRACE(logger_,
                 "No table group. (relay_parent={}, group_id={})",
                 relay_parent,
                 data.group_id);
      }

      const auto v_threshold = std::min(len, size_t(minimum_backing_votes));
      return attested(data.candidate, data, v_threshold);
    } else {
      SL_TRACE(logger_, "No candidate info. (relay_parent={})", relay_parent);
    }
    return std::nullopt;
  }

  std::optional<BackingStore::BackedCandidate>
  ParachainProcessorImpl::table_attested_to_backed(AttestedCandidate &&attested,
                                                   TableContext &table_context,
                                                   bool inject_core_index) {
    const auto core_index = attested.group_id;
    auto it = table_context.groups.find(core_index);
    if (it == table_context.groups.end()) {
      return std::nullopt;
    }

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
    std::sort(vote_positions.begin(),
              vote_positions.end(),
              [](const auto &l, const auto &r) { return l.second < r.second; });

    std::vector<network::ValidityAttestation> validity_votes;
    validity_votes.reserve(vote_positions.size());
    for (const auto &[pos_in_votes, _pos_in_group] : vote_positions) {
      validity_votes.emplace_back(
          std::move(attested.validity_votes[pos_in_votes].second));
    }

    return BackingStore::BackedCandidate::from(
        std::move(attested.candidate),
        std::move(validity_votes),
        std::move(validator_indices),
        inject_core_index ? std::optional<CoreIndex>{core_index}
                          : std::optional<CoreIndex>{});
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

    auto core = core_index_from_statement(rp_state.validator_to_group,
                                          rp_state.group_rotation_info,
                                          rp_state.availability_cores,
                                          statement);
    if (!core) {
      return Error::CORE_INDEX_UNAVAILABLE;
    };

    return importStatementToTable(
        relay_parent, rp_state, *core, candidate_hash, stmnt);
  }

  std::optional<CoreIndex> ParachainProcessorImpl::core_index_from_statement(
      const std::vector<std::optional<GroupIndex>> &validator_to_group,
      const runtime::GroupDescriptor &group_rotation_info,
      const std::vector<runtime::CoreState> &cores,
      const SignedFullStatementWithPVD &statement) {
    const auto &compact_statement = getPayload(statement);
    const auto candidate_hash = candidateHashFrom(compact_statement);

    const auto n_cores = cores.size();
    SL_TRACE(
        logger_,
        "Extracting core index from statement. (candidate_hash={}, n_cores={})",
        candidate_hash,
        n_cores);

    const auto statement_validator_index = statement.payload.ix;
    if (statement_validator_index >= validator_to_group.size()) {
      SL_TRACE(
          logger_,
          "Invalid validator index. (candidate_hash={}, validator_to_group={}, "
          "statement_validator_index={}, n_cores={})",
          candidate_hash,
          validator_to_group.size(),
          statement_validator_index,
          n_cores);
      return std::nullopt;
    }

    const auto group_index = validator_to_group[statement_validator_index];
    if (!group_index) {
      SL_TRACE(logger_,
               "Invalid validator index. Empty group. (candidate_hash={}, "
               "statement_validator_index={}, n_cores={})",
               candidate_hash,
               statement_validator_index,
               n_cores);
      return std::nullopt;
    }

    const auto core_index =
        group_rotation_info.coreForGroup(*group_index, n_cores);

    if (size_t(core_index) > n_cores) {
      SL_WARN(logger_,
              "Invalid CoreIndex. (candidate_hash={}, core_index={}, "
              "validator={}, n_cores={})",
              candidate_hash,
              core_index,
              statement_validator_index,
              n_cores);
      return std::nullopt;
    }

    if (auto s =
            if_type<const StatementWithPVDSeconded>(getPayload(statement))) {
      const auto candidate_para_id =
          s->get().committed_receipt.descriptor.para_id;
      std::optional<ParachainId> assigned_para_id = visit_in_place(
          cores[core_index],
          [&](const runtime::OccupiedCore &occupied)
              -> std::optional<ParachainId> {
            if (occupied.next_up_on_available) {
              return occupied.next_up_on_available->para_id;
            }
            return std::nullopt;
          },
          [&](const runtime::ScheduledCore &scheduled)
              -> std::optional<ParachainId> { return scheduled.para_id; },
          [&](const runtime::FreeCore &) -> std::optional<ParachainId> {
            SL_TRACE(
                logger_,
                "Invalid CoreIndex, core is not assigned to any para_id. "
                "(candidate_hash={}, core_index={}, validator={}, n_cores={})",
                candidate_hash,
                core_index,
                statement_validator_index,
                n_cores);
            return std::nullopt;
          });

      if (!assigned_para_id) {
        return std::nullopt;
      }

      if (*assigned_para_id != candidate_para_id) {
        SL_TRACE(logger_,
                 "Invalid CoreIndex, core is assigned to a different para_id. "
                 "(candidate_hash={}, core_index={}, validator={}, n_cores={})",
                 candidate_hash,
                 core_index,
                 statement_validator_index,
                 n_cores);
        return std::nullopt;
      }
      return core_index;
    }
    return core_index;
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
          {{para_id, {{para_head, std::move(*unblocked)}}}});
    }
  }

  void ParachainProcessorImpl::requestUnblockedCollations(
      std::unordered_map<
          ParachainId,
          std::unordered_map<Hash, std::vector<BlockedAdvertisement>>>
          &&blocked) {
    for (const auto &[para_id, v] : blocked) {
      for (const auto &[para_head, blocked_tmp] : v) {
        std::vector<BlockedAdvertisement> blocked_vec;
        for (const auto &blocked : blocked_tmp) {
          const auto is_seconding_allowed =
              canSecond(para_id,
                        blocked.candidate_relay_parent,
                        blocked.candidate_hash,
                        para_head);
          if (is_seconding_allowed) {
            auto result =
                enqueueCollation(blocked.candidate_relay_parent,
                                 para_id,
                                 blocked.peer_id,
                                 blocked.collator_id,
                                 std::make_optional(std::make_pair(
                                     blocked.candidate_hash, para_head)));
            if (result.has_error()) {
              SL_DEBUG(logger_,
                       "Enqueue collation failed.(candidate={}, para id={}, "
                       "relay_parent={}, para_head={}, peer_id={})",
                       blocked.candidate_hash,
                       para_id,
                       blocked.candidate_relay_parent,
                       para_head,
                       blocked.peer_id);
            }
          } else {
            blocked_vec.emplace_back(blocked);
          }
        }

        if (!blocked_vec.empty()) {
          our_current_state_.blocked_advertisements[para_id][para_head] =
              std::move(blocked_vec);
        }
      }
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
             "Import result.(candidate={}, para id={}, validity votes={})",
             summary->candidate,
             summary->group_id,
             summary->validity_votes);

    if (auto attested = attested_candidate(relay_parent,
                                           summary->candidate,
                                           rp_state.table_context,
                                           rp_state.minimum_backing_votes)) {
      const auto candidate_hash{candidateHash(*hasher_, attested->candidate)};

      if (rp_state.backed_hashes.insert(candidate_hash).second) {
        if (auto backed =
                table_attested_to_backed(std::move(*attested),
                                         rp_state.table_context,
                                         rp_state.inject_core_index)) {
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
        } else {
          SL_TRACE(logger_,
                   "Cannot get BackedCandidate. (candidate_hash={})",
                   candidate_hash);
        }
      } else {
        SL_TRACE(logger_,
                 "Candidate already known. (candidate_hash={})",
                 candidate_hash);
      }
    } else {
      SL_TRACE(logger_, "No attested candidate.");
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
          sign_result.error());
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

  void ParachainProcessorImpl::notify_collation_seconded(
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      const RelayHash &relay_parent,
      const SignedFullStatementWithPVD &statement) {
    logger_->info("Send Seconded to collator.(peer={}, relay parent={})",
                  peer_id,
                  relay_parent);

    network::SignedStatement stm = visit_in_place(
        getPayload(statement),
        [&](const StatementWithPVDSeconded &s) -> network::SignedStatement {
          return {
              .payload =
                  {
                      .payload = network::CandidateState{s.committed_receipt},
                      .ix = statement.payload.ix,
                  },
              .signature = statement.signature,
          };
        },
        [&](const StatementWithPVDValid &s) -> network::SignedStatement {
          return {
              .payload =
                  {
                      .payload = network::CandidateState{s.candidate_hash},
                      .ix = statement.payload.ix,
                  },
              .signature = statement.signature,
          };
        });

    pm_->getStreamEngine()->send(
        peer_id,
        router_->getCollationProtocolVStaging(),
        std::make_shared<
            network::WireMessage<network::vstaging::CollatorProtocolMessage>>(
            network::vstaging::CollatorProtocolMessage(
                network::vstaging::CollationMessage(
                    network::vstaging::CollatorProtocolMessageCollationSeconded{
                        .relay_parent = relay_parent,
                        .statement = std::move(stm)}))));
  }

  template <bool kReinvoke>
  void ParachainProcessorImpl::notifyInvalid(
      const primitives::BlockHash &parent,
      const network::CandidateReceipt &candidate_receipt) {
    REINVOKE_ONCE(
        *main_pool_handler_, notifyInvalid, parent, candidate_receipt);

    auto fetched_collation =
        network::FetchedCollation::from(candidate_receipt, *hasher_);
    const auto &candidate_hash = fetched_collation.candidate_hash;

    auto it = our_current_state_.validator_side.fetched_candidates.find(
        fetched_collation);
    if (it == our_current_state_.validator_side.fetched_candidates.end()) {
      return;
    }

    if (!it->second.pending_collation.commitments_hash
        || *it->second.pending_collation.commitments_hash
               != candidate_receipt.commitments_hash) {
      SL_ERROR(logger_,
               "Reported invalid candidate for unknown `pending_candidate`! "
               "(relay_parent={}, candidate_hash={})",
               parent,
               candidate_hash);
      return;
    }

    auto id = it->second.collator_id;
    our_current_state_.validator_side.fetched_candidates.erase(it);

    /// TODO(iceseer): reduce collator's reputation
    dequeue_next_collation_and_fetch(parent, {id, candidate_hash});
  }

  template <bool kReinvoke>
  void ParachainProcessorImpl::notifySeconded(
      const primitives::BlockHash &parent,
      const SignedFullStatementWithPVD &statement) {
    REINVOKE_ONCE(*main_pool_handler_, notifySeconded, parent, statement);

    auto seconded =
        if_type<const StatementWithPVDSeconded>(getPayload(statement));
    if (!seconded) {
      SL_TRACE(logger_,
               "Seconded message received with a `Valid` statement. "
               "(relay_parent={})",
               parent);
      return;
    }

    auto fetched_collation = network::FetchedCollation::from(
        seconded->get().committed_receipt.to_plain(*hasher_), *hasher_);
    auto it = our_current_state_.validator_side.fetched_candidates.find(
        fetched_collation);
    if (it == our_current_state_.validator_side.fetched_candidates.end()) {
      SL_TRACE(logger_,
               "Collation has been seconded, but the relay parent is "
               "deactivated. (relay_parent={})",
               parent);
      return;
    }

    auto collation_event{std::move(it->second)};
    our_current_state_.validator_side.fetched_candidates.erase(it);

    auto &collator_id = collation_event.collator_id;
    auto &pending_collation = collation_event.pending_collation;

    auto &relay_parent = pending_collation.relay_parent;
    auto &peer_id = pending_collation.peer_id;
    auto &prospective_candidate = pending_collation.prospective_candidate;

    if (auto peer_data = pm_->getPeerState(peer_id)) {
      network::CollationVersion version = network::CollationVersion::VStaging;
      if (peer_data->get().version) {
        version = *peer_data->get().version;
      }
      notify_collation_seconded(peer_id, version, relay_parent, statement);
    }

    if (auto rp_state = tryGetStateByRelayParent(parent)) {
      rp_state->get().collations.status = CollationStatus::Seconded;
      rp_state->get().collations.note_seconded();
    }

    const auto maybe_candidate_hash = utils::map(
        prospective_candidate, [](const auto &v) { return v.candidate_hash; });

    dequeue_next_collation_and_fetch(parent,
                                     {collator_id, maybe_candidate_hash});

    /// TODO(iceseer): Bump collator reputation
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
      const ValidateAndSecondResult &validation_result) {
    logger_->trace("On validation complete. (relay parent={})",
                   validation_result.relay_parent);

    auto opt_parachain_state =
        tryGetStateByRelayParent(validation_result.relay_parent);
    if (!opt_parachain_state) {
      logger_->trace("Validated candidate from {} out of view",
                     validation_result.relay_parent);
      return;
    }

    auto &parachain_state = opt_parachain_state->get();
    const auto candidate_hash = validation_result.candidate.hash(*hasher_);

    if (!validation_result.result) {
      SL_WARN(logger_,
              "Candidate {} validation failed with: {}",
              candidate_hash,
              validation_result.result.error());
      notifyInvalid(validation_result.candidate.descriptor.relay_parent,
                    validation_result.candidate);
      return;
    }

    if (parachain_state.issued_statements.contains(candidate_hash)) {
      return;
    }

    logger_->trace("Second candidate complete. (candidate={}, relay parent={})",
                   candidate_hash,
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

    auto res = sign_import_and_distribute_statement<StatementType::kSeconded>(
        parachain_state, validation_result);
    if (res.has_error()) {
      SL_WARN(logger_,
              "Attempted to second candidate but was rejected by prospective "
              "parachains. (candidate_hash={}, relay_parent={}, error={})",
              candidate_hash,
              validation_result.relay_parent,
              res.error());
      notifyInvalid(validation_result.candidate.descriptor.relay_parent,
                    validation_result.candidate);
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
    notifySeconded(validation_result.relay_parent, stmt);
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
             "Sharing statement. (relay parent={}, candidate hash={}, "
             "our_index={}, statement_ix={})",
             relay_parent,
             candidate_hash,
             *per_relay_parent.our_index,
             statement.payload.ix);

    BOOST_ASSERT(per_relay_parent.our_index);

    const Groups &groups = per_relay_parent.per_session_state->value().groups;
    const std::optional<network::ParachainId> &local_assignment =
        per_relay_parent.assigned_para;
    const network::ValidatorIndex local_index = *per_relay_parent.our_index;
    const auto local_group_opt = groups.byValidatorIndex(local_index);
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

    if (per_relay_parent.local_validator
        && per_relay_parent.local_validator->active) {
      per_relay_parent.local_validator->active->cluster_tracker.note_issued(
          local_index, network::vstaging::from(getPayload(compact_statement)));
    }

    if (per_relay_parent.per_session_state->value().grid_view) {
      auto &l = *per_relay_parent.local_validator;
      l.grid_tracker.learned_fresh_statement(
          per_relay_parent.per_session_state->value().groups,
          *per_relay_parent.per_session_state->value().grid_view,
          local_index,
          getPayload(compact_statement));
    }

    circulate_statement(relay_parent, per_relay_parent, compact_statement);
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
      const primitives::BlockHash &candidate_hash,
      ValidateAndSecondResult &&validate_and_second_result) {
    REINVOKE(*main_pool_handler_,
             makeAvailable<kMode>,
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
      onValidationComplete(std::move(q));
    } else {
      onAttestComplete(std::move(q));
    }
  }

  template <ParachainProcessorImpl::ValidationTaskType kMode>
  void ParachainProcessorImpl::validateAsync(
      network::CandidateReceipt &&candidate,
      network::ParachainBlock &&pov,
      runtime::PersistedValidationData &&pvd,
      const primitives::BlockHash &relay_parent) {
    REINVOKE(*main_pool_handler_,
             validateAsync<kMode>,
             std::move(candidate),
             std::move(pov),
             std::move(pvd),
             relay_parent);

    auto parachain_state =
        tryGetStateByRelayParent(candidate.descriptor.relay_parent);
    if (!parachain_state) {
      return;
    }

    const auto candidate_hash{candidate.hash(*hasher_)};
    if constexpr (kMode == ValidationTaskType::kAttest) {
      if (parachain_state->get().issued_statements.contains(candidate_hash)) {
        return;
      }
    }

    if (!parachain_state->get()
             .awaiting_validation.insert(candidate_hash)
             .second) {
      return;
    }

    SL_INFO(logger_,
            "Starting validation task.(para id={}, "
            "relay parent={}, candidate_hash={})",
            candidate.descriptor.para_id,
            relay_parent,
            candidate_hash);

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// checks if we still need to execute parachain task
    auto _measure = std::make_shared<TicToc>("Parachain validation", logger_);
    auto cb = [weak_self{weak_from_this()},
               candidate,
               pov,
               pvd,
               relay_parent,
               n_validators{
                   parachain_state->get().table_context.validators.size()},
               _measure,
               candidate_hash](
                  outcome::result<Pvf::Result> validation_result) mutable {
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      if (!validation_result) {
        SL_WARN(self->logger_,
                "Candidate {} on relay_parent {}, para_id {} validation failed "
                "with "
                "error: {}",
                candidate_hash,
                candidate.descriptor.relay_parent,
                candidate.descriptor.para_id,
                validation_result.error().message());
        return;
      }

      auto &[comms, data] = validation_result.value();
      runtime::AvailableData available_data{
          .pov = std::move(pov),
          .validation_data = std::move(data),
      };

      auto measure2 = std::make_shared<TicToc>("===> EC validation", self->logger_);
      auto chunks_res =
          self->validateErasureCoding(available_data, n_validators);
      if (chunks_res.has_error()) {
        SL_WARN(self->logger_,
                "Erasure coding validation failed. (error={})",
                chunks_res.error());
        return;
      }
      measure2.reset();
      auto &chunks = chunks_res.value();

      self->notifyAvailableData(std::move(chunks),
                                relay_parent,
                                candidate_hash,
                                available_data.pov,
                                available_data.validation_data);

      self->makeAvailable<kMode>(
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
      const ValidateAndSecondResult &result) {
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

  void ParachainProcessorImpl::prune_old_advertisements(
      const parachain::ImplicitView &implicit_view,
      const std::unordered_map<Hash, ProspectiveParachainsModeOpt>
          &active_leaves,
      const std::unordered_map<primitives::BlockHash, RelayParentState>
          &per_relay_parent) {
    pm_->enumeratePeerState(
        [&](const libp2p::peer::PeerId &peer, network::PeerState &ps) {
          if (ps.collator_state) {
            auto &peer_state = *ps.collator_state;
            for (auto it = peer_state.advertisements.begin();
                 it != peer_state.advertisements.end();) {
              const auto &hash = it->first;

              bool res = false;
              if (auto i = per_relay_parent.find(hash);
                  i != per_relay_parent.end()) {
                const auto &s = i->second;
                res = isRelayParentInImplicitView(hash,
                                                  s.prospective_parachains_mode,
                                                  implicit_view,
                                                  active_leaves,
                                                  peer_state.para_id);
              }

              if (res) {
                ++it;
              } else {
                it = peer_state.advertisements.erase(it);
              }
            }
          }

          return true;
        });
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

  outcome::result<void> ParachainProcessorImpl::kick_off_seconding(
      network::PendingCollationFetch &&pending_collation_fetch) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    auto &collation_event = pending_collation_fetch.collation_event;
    auto pending_collation = collation_event.pending_collation;
    auto relay_parent = pending_collation.relay_parent;

    OUTCOME_TRY(per_relay_parent, getStateByRelayParent(relay_parent));
    auto &collations = per_relay_parent.get().collations;
    auto fetched_collation = network::FetchedCollation::from(
        pending_collation_fetch.candidate_receipt, *hasher_);

    if (our_current_state_.validator_side.fetched_candidates.contains(
            fetched_collation)) {
      return Error::DUPLICATE;
    }

    collation_event.pending_collation.commitments_hash =
        pending_collation_fetch.candidate_receipt.commitments_hash;

    const bool is_collator_v2 = (collation_event.collator_protocol_version
                                 == network::CollationVersion::VStaging);
    const bool have_prospective_candidate =
        collation_event.pending_collation.prospective_candidate.has_value();
    const bool async_backing_en =
        per_relay_parent.get().prospective_parachains_mode.has_value();

    std::optional<runtime::PersistedValidationData> maybe_pvd;
    std::optional<std::pair<HeadData, Hash>> maybe_parent_head_and_hash;
    if (is_collator_v2 && have_prospective_candidate && async_backing_en) {
      OUTCOME_TRY(pvd,
                  requestProspectiveValidationData(
                      relay_parent,
                      collation_event.pending_collation.prospective_candidate
                          ->parent_head_data_hash,
                      pending_collation.para_id,
                      pending_collation_fetch.maybe_parent_head_data));
      maybe_pvd = pvd;

      if (pending_collation_fetch.maybe_parent_head_data) {
        maybe_parent_head_and_hash.emplace(
            *pending_collation_fetch.maybe_parent_head_data,
            collation_event.pending_collation.prospective_candidate
                ->parent_head_data_hash);
      }
    } else if ((is_collator_v2 && have_prospective_candidate)
               || !is_collator_v2) {
      OUTCOME_TRY(
          pvd,
          requestPersistedValidationData(
              pending_collation_fetch.candidate_receipt.descriptor.relay_parent,
              pending_collation_fetch.candidate_receipt.descriptor.para_id));
      maybe_pvd = pvd;
      maybe_parent_head_and_hash = std::nullopt;
    } else {
      return outcome::success();
    }

    if (!maybe_pvd) {
      return Error::PERSISTED_VALIDATION_DATA_NOT_FOUND;
    }

    auto pvd{std::move(*maybe_pvd)};
    OUTCOME_TRY(fetched_collation_sanity_check(
        collation_event.pending_collation,
        pending_collation_fetch.candidate_receipt,
        pvd,
        maybe_parent_head_and_hash));

    collations.status = CollationStatus::WaitingOnValidation;
    validateAsync<ValidationTaskType::kSecond>(
        std::move(pending_collation_fetch.candidate_receipt),
        std::move(pending_collation_fetch.pov),
        std::move(pvd),
        relay_parent);

    our_current_state_.validator_side.fetched_candidates.emplace(
        fetched_collation, collation_event);
    return outcome::success();
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
    [[maybe_unused]] const auto candidate_hash =
        candidateHash(hypothetical_candidate);

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

  bool ParachainProcessorImpl::canSecond(ParachainId candidate_para_id,
                                         const Hash &relay_parent,
                                         const CandidateHash &candidate_hash,
                                         const Hash &parent_head_data_hash) {
    auto per_relay_parent = tryGetStateByRelayParent(relay_parent);
    if (per_relay_parent) {
      if (per_relay_parent->get().prospective_parachains_mode) {
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
    }
    return false;
  }

  void ParachainProcessorImpl::handleAdvertisement(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate) {
    REINVOKE(*main_pool_handler_,
             handleAdvertisement,
             relay_parent,
             peer_id,
             std::move(prospective_candidate));

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
        per_relay_parent.assigned_para;

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
    if (!assignment || collator_para_id != *assignment) {
      SL_TRACE(logger_,
               "Invalid assignment. (peerd_id={}, collator={})",
               peer_id,
               collator_para_id);
      return;
    }

    // Check for protocol mismatch
    if (relay_parent_mode && !prospective_candidate) {
      SL_WARN(logger_, "Protocol mismatch. (peer_id={})", peer_id);
      return;
    }

    const auto candidate_hash =
        utils::map(prospective_candidate,
                   [](const auto &pair) { return std::cref(pair.first); });

    // Try to insert the advertisement
    auto insert_res = insertAdvertisement(
        peer_state->get(), relay_parent, relay_parent_mode, candidate_hash);
    if (insert_res.has_error()) {
      // If there is an error inserting the advertisement, log a trace message
      // and return
      SL_TRACE(logger_,
               "Insert advertisement error. (error={})",
               insert_res.error());
      return;
    }

    // Get the collator id and parachain id from the result of the advertisement
    // insertion
    const auto &[collator_id, para_id] = insert_res.value();
    if (!per_relay_parent.collations.hasSecondedSpace(relay_parent_mode)) {
      SL_TRACE(logger_, "Seconded limit reached.");
      return;
    }

    if (prospective_candidate) {
      auto &&[ch, parent_head_data_hash] = *prospective_candidate;
      const bool queue_advertisement =
          relay_parent_mode
          && !canSecond(
              collator_para_id, relay_parent, ch, parent_head_data_hash);

      if (queue_advertisement) {
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

    if (auto result = enqueueCollation(relay_parent,
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
               result.error());
    }
  }

  outcome::result<void> ParachainProcessorImpl::enqueueCollation(
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

    auto per_relay_parent = tryGetStateByRelayParent(relay_parent);
    if (!per_relay_parent) {
      SL_TRACE(logger_,
               "Candidate relay parent went out of view for valid "
               "advertisement. (peer id={}, para id={}, relay parent={})",
               peer_id,
               para_id,
               relay_parent);
      return outcome::success();
    }

    const auto &relay_parent_mode =
        per_relay_parent->get().prospective_parachains_mode;
    auto &collations = per_relay_parent->get().collations;

    if (!collations.hasSecondedSpace(relay_parent_mode)) {
      SL_TRACE(logger_,
               "Limit of seconded collations reached for valid advertisement. "
               "(peer={}, para id={}, relay parent={})",
               peer_id,
               para_id,
               relay_parent);
      return outcome::success();
    }

    const auto pc = utils::map(prospective_candidate, [](const auto &p) {
      return network::ProspectiveCandidate{
          .candidate_hash = p.first,
          .parent_head_data_hash = p.second,
      };
    });

    PendingCollation pending_collation{
        .relay_parent = relay_parent,
        .para_id = para_id,
        .peer_id = peer_id,
        .prospective_candidate = std::move(pc),
        .commitments_hash = {},
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
        std::ignore = fetchCollation(std::move(pending_collation), collator_id);
      } break;
      case CollationStatus::Seconded: {
        if (relay_parent_mode) {
          // Limit is not reached, it's allowed to second another
          // collation.
          std::ignore =
              fetchCollation(std::move(pending_collation), collator_id);
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

  outcome::result<void> ParachainProcessorImpl::fetchCollation(
      const PendingCollation &pc, const CollatorId &id) {
    auto peer_state = pm_->getPeerState(pc.peer_id);
    if (!peer_state) {
      SL_TRACE(
          logger_, "No peer state. Unknown peer. (peer id={})", pc.peer_id);
      return Error::NO_PEER;
    }

    const auto candidate_hash =
        utils::map(pc.prospective_candidate,
                   [](const auto &v) { return std::cref(v.candidate_hash); });

    network::CollationVersion version = network::CollationVersion::VStaging;
    if (peer_state->get().version) {
      version = *peer_state->get().version;
    }

    if (peer_state->get().hasAdvertised(pc.relay_parent, candidate_hash)) {
      return fetchCollation(pc, id, version);
    }
    SL_WARN(logger_, "Not advertised. (peer id={})", pc.peer_id);
    return Error::NOT_ADVERTISED;
  }

  outcome::result<void> ParachainProcessorImpl::fetchCollation(
      const PendingCollation &pc,
      const CollatorId &id,
      network::CollationVersion version) {
    if (our_current_state_.collation_requests_cancel_handles.count(pc)
        != 0ull) {
      SL_WARN(logger_,
              "Already requested. (relay parent={}, para id={})",
              pc.relay_parent,
              pc.para_id);
      return Error::ALREADY_REQUESTED;
    }

    auto per_relay_parent = tryGetStateByRelayParent(pc.relay_parent);
    if (!per_relay_parent) {
      return Error::OUT_OF_VIEW;
    }

    network::CollationEvent collation_event{
        .collator_id = id,
        .collator_protocol_version = version,
        .pending_collation = pc,
    };

    const auto peer_id = pc.peer_id;
    auto response_callback =
        [collation_event{std::move(collation_event)}, wptr{weak_from_this()}](
            outcome::result<network::CollationFetchingResponse>
                &&result) mutable {
          auto self = wptr.lock();
          if (!self) {
            return;
          }

          const RelayHash &relay_parent =
              collation_event.pending_collation.relay_parent;
          const libp2p::peer::PeerId &peer_id =
              collation_event.pending_collation.peer_id;

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
            return;
          }

          self->handle_collation_fetch_response(std::move(collation_event),
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
                   [](const auto &v) { return std::cref(v.candidate_hash); });
    per_relay_parent->get().collations.status = CollationStatus::Fetching;
    per_relay_parent->get().collations.fetching_from.emplace(
        id, maybe_candidate_hash);

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
    return outcome::success();
  }

}  // namespace kagome::parachain

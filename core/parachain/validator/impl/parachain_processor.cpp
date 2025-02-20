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
#include "network/impl/protocols/parachain.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"
#include "parachain/availability/chunks.hpp"
#include "parachain/availability/proof.hpp"
#include "parachain/candidate_descriptor_v2.hpp"
#include "parachain/candidate_view.hpp"
#include "parachain/peer_relay_parent_knowledge.hpp"
#include "scale/scale.hpp"
#include "utils/async_sequence.hpp"
#include "utils/map.hpp"
#include "utils/pool_handler.hpp"
#include "utils/profiler.hpp"
#include "utils/weak_macro.hpp"

#ifndef TRY_GET_OR_RET
#define TRY_GET_OR_RET(name, op) \
  auto name = (op);              \
  if (!name) {                   \
    return;                      \
  }
#endif  // TRY_GET_OR_RET

#ifndef CHECK_OR_RET
#define CHECK_OR_RET(op) \
  if (!(op)) {           \
    return;              \
  }
#endif  // CHECK_OR_RET

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
    case E::THRESHOLD_LIMIT_REACHED:
      return "Threshold reached";
  }
  return "Unknown parachain processor error";
}

namespace {
  constexpr const char *kIsParachainValidator =
      "kagome_node_is_parachain_validator";
  constexpr auto kImplicitVotes = "kagome_parachain_implicit_votes";
  constexpr auto kExplicitVotes = "kagome_parachain_explicit_votes";
  constexpr auto kNoVotes = "kagome_parachain_no_votes";
  constexpr auto kSessionIndex = "kagome_session_index";
  constexpr auto parachain_inherent_data_extrinsic_version = 0x04;
  constexpr auto parachain_inherent_data_call = 0x36;
  constexpr auto parachain_inherent_data_module = 0x00;
}  // namespace

namespace kagome::parachain {

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
      primitives::events::SyncStateSubscriptionEnginePtr sync_state_observable,
      std::shared_ptr<authority_discovery::Query> query_audi,
      std::shared_ptr<ProspectiveParachains> prospective_parachains,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      LazySPtr<consensus::SlotsUtil> slots_util,
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      std::shared_ptr<statement_distribution::StatementDistribution> sd)
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
        sync_state_observable_(std::move(sync_state_observable)),
        query_audi_{std::move(query_audi)},
        slots_util_(slots_util),
        babe_config_repo_(std::move(babe_config_repo)),
        chain_sub_{std::move(chain_sub_engine)},
        worker_pool_handler_{worker_thread_pool.handler(app_state_manager)},
        prospective_parachains_{std::move(prospective_parachains)},
        block_tree_{std::move(block_tree)},
        statement_distribution(std::move(sd)),
        per_session(RefCache<SessionIndex, PerSessionState>::create()) {
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
    BOOST_ASSERT(sync_state_observable_);
    BOOST_ASSERT(query_audi_);
    BOOST_ASSERT(prospective_parachains_);
    BOOST_ASSERT(worker_pool_handler_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(statement_distribution);
    app_state_manager.takeControl(*this);

    our_current_state_.implicit_view.emplace(
        prospective_parachains_, parachain_host_, block_tree_, std::nullopt);
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

    metrics_registry_->registerCounterFamily(
        "kagome_parachain_candidate_backing_signed_statements_total",
        "Block height info of the chain");
    metric_kagome_parachain_candidate_backing_signed_statements_total_ =
        metrics_registry_->registerCounterMetric(
            "kagome_parachain_candidate_backing_signed_statements_total");

    metrics_registry_->registerCounterFamily(
        "kagome_parachain_candidate_backing_candidates_seconded_total",
        "Number of candidates seconded");
    metric_kagome_parachain_candidate_backing_candidates_seconded_total_ =
        metrics_registry_->registerCounterMetric(
            "kagome_parachain_candidate_backing_candidates_seconded_total");

    metrics_registry_->registerGaugeFamily(kSessionIndex,
                                           "Parachain session index");
    metric_session_index_ =
        metrics_registry_->registerGaugeMetric(kSessionIndex);
    metric_session_index_->set(0);
    metrics_registry_->registerCounterFamily(
        kImplicitVotes, "Implicit votes for parachain candidates");
    metric_kagome_parachain_candidate_implicit_votes_total_ =
        metrics_registry_->registerCounterMetric(kImplicitVotes);
    metrics_registry_->registerCounterFamily(
        kExplicitVotes, "Explicit votes for parachain candidates");
    metric_kagome_parachain_candidate_explicit_votes_total_ =
        metrics_registry_->registerCounterMetric(kExplicitVotes);
    metrics_registry_->registerCounterFamily(
        kNoVotes, "No votes for parachain candidates");
    metric_kagome_parachain_candidate_no_votes_total_ =
        metrics_registry_->registerCounterMetric(kNoVotes);
  }

  void ParachainProcessorImpl::OnBroadcastBitfields(
      const primitives::BlockHash &relay_parent,
      const network::SignedBitfield &bitfield) {
    REINVOKE(*main_pool_handler_, OnBroadcastBitfields, relay_parent, bitfield);
    SL_TRACE(logger_, "Distribute bitfield on {}", relay_parent);
    router_->getValidationProtocol()->write(network::BitfieldDistribution{
        .relay_parent = relay_parent,
        .data = bitfield,
    });
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
    statement_distribution->store_parachain_processor(weak_from_this());
    // Set the broadcast callback for the bitfield signer
    bitfield_signer_->setBroadcastCallback(
        [wptr_self{weak_from_this()}](const primitives::BlockHash &relay_parent,
                                      const network::SignedBitfield &bitfield) {
          TRY_GET_OR_RET(self, wptr_self.lock());
          self->OnBroadcastBitfields(relay_parent, bitfield);
        });

    // Subscribe to the BABE status observable
    sync_state_observer_ =
        primitives::events::onSync(sync_state_observable_, [WEAK_SELF] {
          WEAK_LOCK(self);
          self->synchronized_ = true;
          self->bitfield_signer_->start();
          self->pvf_precheck_->start();
        });

    // Set the callback for the my view observable
    // This callback is triggered when the kViewUpdate event is fired.
    // It updates the active leaves, checks if parachains can be processed,
    // creates a new backing task for the new head, and broadcasts the updated
    // view.
    my_view_sub_ = primitives::events::subscribe(
        peer_view_->getMyViewObservable(),
        network::PeerView::EventType::kViewUpdated,
        [WEAK_SELF](const network::ExView &event) {
          WEAK_LOCK(self);
          self->onViewUpdated(event);
        });

    chain_sub_.onFinalize([WEAK_SELF] {
      WEAK_LOCK(self);
      if (self) {
        self->onFinalize();
      }
    });
    return true;
  }

  void ParachainProcessorImpl::onViewUpdated(const network::ExView &event) {
    REINVOKE(*main_pool_handler_, onViewUpdated, event);
    CHECK_OR_RET(canProcessParachains().has_value());
    const auto &relay_parent = event.new_head.hash();

    /// init `prospective_parachains` subsystem
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
              r.error());
    }

    /// init `backing_store` subsystem
    backing_store_->onActivateLeaf(relay_parent);

    /// init `backing` subsystem
    auto pruned = create_backing_task(relay_parent, event.new_head, event.lost);

    SL_TRACE(logger_,
             "Update my view.(new head={}, finalized={}, leaves={})",
             relay_parent,
             event.view.finalized_number_,
             event.view.heads_.size());

    handle_active_leaves_update_for_validator(event, std::move(pruned));
  }

  void ParachainProcessorImpl::handle_active_leaves_update_for_validator(
      const network::ExView &event, std::vector<Hash> pruned_h) {
    const auto current_leaves = our_current_state_.validator_side.active_leaves;
    std::unordered_map<Hash, ProspectiveParachainsModeOpt> removed;
    for (const auto &[h, m] : current_leaves) {
      if (!event.view.contains(h)) {
        removed.emplace(h, m);
      }
    }
    std::vector<Hash> added;
    for (const auto &h : event.view.heads_) {
      if (!current_leaves.contains(h)) {
        added.emplace_back(h);
      }
    }

    for (const auto &leaf : added) {
      const auto mode =
          prospective_parachains_->prospectiveParachainsMode(leaf);
      our_current_state_.validator_side.active_leaves[leaf] = mode;
    }

    for (const auto &[removed, mode] : removed) {
      our_current_state_.validator_side.active_leaves.erase(removed);
      const std::vector<Hash> pruned =
          mode ? std::move(pruned_h) : std::vector<Hash>{removed};

      for (const auto &removed : pruned) {
        auto it = our_current_state_.state_by_relay_parent.find(removed);
        if (it != our_current_state_.state_by_relay_parent.end()) {
          const auto &relay_parent = it->first;
          state_by_relay_parent_to_check_[relay_parent] = std::move(it->second);
          if (auto block_number = block_tree_->getNumberByHash(relay_parent)) {
            relay_parent_depth_[relay_parent] = block_number.value();
          } else {
            SL_DEBUG(logger_,
                     "Failed to get block number while pruning relay parent "
                     "state. (relay_parent={})",
                     relay_parent);
          }
          our_current_state_.state_by_relay_parent.erase(it);
        }

        {  /// remove cancelations
          auto &container =
              our_current_state_.collation_requests_cancel_handles;
          for (auto pc = container.begin(); pc != container.end();) {
            if (pc->relay_parent != removed) {
              ++pc;
            } else {
              pc = container.erase(pc);
            }
          }
        }
        {  /// remove fetched candidates
          auto &container =
              our_current_state_.validator_side.fetched_candidates;
          for (auto pc = container.begin(); pc != container.end();) {
            if (pc->first.relay_parent != removed) {
              ++pc;
            } else {
              pc = container.erase(pc);
            }
          }
        }
      }
    }

    retain_if(our_current_state_.validator_side.blocked_from_seconding,
              [&](auto &pair) {
                auto &collations = pair.second;
                retain_if(collations, [&](const auto &collation) {
                  return our_current_state_.state_by_relay_parent.contains(
                      collation.candidate_receipt.descriptor.relay_parent);
                });
                return !collations.empty();
              });

    prune_old_advertisements(*our_current_state_.implicit_view,
                             our_current_state_.validator_side.active_leaves,
                             our_current_state_.state_by_relay_parent);
    printStoragesLoad();
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
    if (not synchronized_) {
      return Error::NOT_SYNCHRONIZED;
    }
    return outcome::success();
  }

  outcome::result<consensus::Randomness>
  ParachainProcessorImpl::getBabeRandomness(const RelayHash &relay_parent) {
    OUTCOME_TRY(block_header, block_tree_->getBlockHeader(relay_parent));
    OUTCOME_TRY(babe_header, consensus::babe::getBabeBlockHeader(block_header));
    OUTCOME_TRY(epoch,
                slots_util_.get()->slotToEpoch(*block_header.parentInfo(),
                                               babe_header.slot_number));
    OUTCOME_TRY(babe_config,
                babe_config_repo_->config(*block_header.parentInfo(), epoch));

    return babe_config->randomness;
  }

  outcome::result<kagome::parachain::ParachainProcessorImpl::RelayParentState>
  ParachainProcessorImpl::construct_per_relay_parent_state(
      const primitives::BlockHash &relay_parent,
      const ProspectiveParachainsModeOpt &mode) {
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
    ::libp2p::common::FinalAction metric_updater{
        [&] { metric_is_parachain_validator_->set(is_parachain_validator); }};
    OUTCOME_TRY(validators, parachain_host_->validators(relay_parent));
    OUTCOME_TRY(groups, parachain_host_->validator_groups(relay_parent));
    OUTCOME_TRY(cores, parachain_host_->availability_cores(relay_parent));
    OUTCOME_TRY(validator, isParachainValidator(relay_parent));
    OUTCOME_TRY(session_index,
                parachain_host_->session_index_for_child(relay_parent));
    OUTCOME_TRY(session_info,
                parachain_host_->session_info(relay_parent, session_index));
    const auto &[validator_groups, group_rotation_info] = groups;

    if (!validator) {
      SL_TRACE(logger_, "Not a parachain validator, or no para keys.");
    } else {
      is_parachain_validator = true;
    }

    if (!session_info) {
      return Error::NO_SESSION_INFO;
    }

    OUTCOME_TRY(node_features, parachain_host_->node_features(relay_parent));
    auto inject_core_index =
        node_features.has(runtime::NodeFeatures::ElasticScalingMVP);

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

    std::optional<ValidatorIndex> validator_index;
    // https://github.com/paritytech/polkadot-sdk/blob/1e3b8e1639c1cf784eabf0a9afcab1f3987e0ca4/polkadot/node/network/collator-protocol/src/validator_side/mod.rs#L487-L495
    CoreIndex current_core = 0;
    if (validator) {
      validator_index = validator->validatorIndex();

      size_t i_group = 0;
      for (auto &group : validator_groups) {
        if (group.contains(validator->validatorIndex())) {
          current_core =
              group_rotation_info.coreForGroup(i_group, cores.size());
          break;
        }
        ++i_group;
      }
    }

    OUTCOME_TRY(maybe_claim_queue, parachain_host_->claim_queue(relay_parent));

    OUTCOME_TRY(global_v_index,
                signer_factory_->getAuthorityValidatorIndex(relay_parent));
    if (!global_v_index) {
      SL_TRACE(logger_, "Not a validator, or no para keys.");
      return Error::NOT_A_VALIDATOR;
    }

    OUTCOME_TRY(per_session_state,
                per_session->get_or_insert(
                    session_index,
                    [&]() -> outcome::result<
                              RefCache<SessionIndex, PerSessionState>::RefObj> {
                      return outcome::success(
                          RefCache<SessionIndex, PerSessionState>::RefObj(
                              session_index, *session_info));
                    }));

    const auto n_cores = cores.size();
    std::unordered_map<CoreIndex, std::vector<ValidatorIndex>> out_groups;
    std::optional<CoreIndex> assigned_core;

    const auto has_claim_queue = maybe_claim_queue.has_value();
    runtime::ClaimQueueSnapshot &claim_queue = *maybe_claim_queue;

    // Iterate over each core index and assign the parachain ID to the node
    for (CoreIndex idx = 0; idx < static_cast<CoreIndex>(cores.size()); ++idx) {
      const auto core_index = idx;
      const auto &core = cores[core_index];

      // If there is no claim queue, determine the parachain ID for the core
      if (!has_claim_queue) {
        std::optional<ParachainId> core_para_id = visit_in_place(
            core,
            // If the core is occupied, get the next parachain ID if available
            [&](const runtime::OccupiedCore &occupied)
                -> std::optional<ParachainId> {
              if (mode && occupied.next_up_on_available) {
                return occupied.next_up_on_available->para_id;
              }
              return std::nullopt;
            },
            // If the core is scheduled, get the parachain ID
            [&](const runtime::ScheduledCore &scheduled)
                -> std::optional<ParachainId> { return scheduled.para_id; },
            // If the core is free, return no parachain ID
            [](const runtime::FreeCore &) -> std::optional<ParachainId> {
              return std::nullopt;
            });
        // If no parachain ID is found, continue to the next core
        if (!core_para_id) {
          continue;
        }
        // Add the parachain ID to the claim queue for the core
        claim_queue.claimes.emplace(core_index,
                                    std::vector<ParachainId>{*core_para_id});
      } else if (!claim_queue.claimes.contains(core_index)) {
        // If the claim queue does not contain the core index, continue to the
        // next core
        continue;
      }

      // Get the group index for the core
      const GroupIndex group_index =
          group_rotation_info.groupForCore(core_index, n_cores);
      // If the group index is valid, process the validator group
      if (group_index < validator_groups.size()) {
        const auto &g = validator_groups[group_index];
        // If the validator index is part of the group, assign the core
        if (validator_index && g.contains(*validator_index)) {
          assigned_core = core_index;
        }
        // Add the core index and its validators to the output groups
        out_groups.emplace(core_index, g.validators);
      }
    }

    std::vector<std::optional<GroupIndex>> validator_to_group;
    validator_to_group.resize(validators.size());
    for (GroupIndex group_idx = 0; group_idx < validator_groups.size();
         ++group_idx) {
      const auto &validator_group = validator_groups[group_idx];
      for (const auto v : validator_group.validators) {
        validator_to_group[v] = group_idx;
      }
    }

    SL_VERBOSE(logger_,
               "Inited new backing task v3.("
               "assigned_core={}, our index={}, relay parent={})",
               assigned_core,
               global_v_index,
               relay_parent);

    return RelayParentState{
        .prospective_parachains_mode = mode,
        .assigned_core = assigned_core,
        .validator_to_group = std::move(validator_to_group),
        .collations = {},
        .table_context =
            TableContext{
                .validator = std::move(validator),
                .groups = std::move(out_groups),
                .validators = std::move(validators),
            },
        .availability_cores = cores,
        .group_rotation_info = group_rotation_info,
        .minimum_backing_votes = minimum_backing_votes,
        .claim_queue = std::move(claim_queue),
        .awaiting_validation = {},
        .issued_statements = {},
        .peers_advertised = {},
        .fallbacks = {},
        .backed_hashes = {},
        .inject_core_index = inject_core_index,
        .v2_receipts =
            node_features.has(runtime::NodeFeatures::CandidateReceiptV2),
        .current_core = current_core,
        .per_session_state = per_session_state,
    };
  }

  std::vector<Hash> ParachainProcessorImpl::create_backing_task(
      const primitives::BlockHash &relay_parent,
      const network::HashedBlockHeader &block_header,
      const std::vector<primitives::BlockHash> &lost) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    using LeafHasProspectiveParachains =
        std::optional<outcome::result<ProspectiveParachainsMode>>;
    LeafHasProspectiveParachains res;

    if (auto mode =
            prospective_parachains_->prospectiveParachainsMode(relay_parent)) {
      if (auto r =
              our_current_state_.implicit_view->activate_leaf(relay_parent);
          r.has_error()) {
        res = r.as_failure();
      } else {
        res = *mode;
      }
    } else {
      res = std::nullopt;
    }

    std::vector<Hash> pruned;
    for (const auto &l : lost) {
      our_current_state_.per_leaf.erase(l);
      pruned = our_current_state_.implicit_view->deactivate_leaf(l);
      backing_store_->onDeactivateLeaf(l);
      bitfield_store_->remove(l);
    }

    std::vector<
        std::shared_ptr<RefCache<SessionIndex, PerSessionState>::RefObj>>
        _keeper_;
    _keeper_.reserve(lost.size());
    {
      std::unordered_set<Hash> remaining;
      for (const auto &[h, _] : our_current_state_.per_leaf) {
        remaining.emplace(h);
      }
      for (const auto &h :
           our_current_state_.implicit_view->all_allowed_relay_parents()) {
        remaining.emplace(h);
      }

      for (auto it = our_current_state_.state_by_relay_parent.begin();
           it != our_current_state_.state_by_relay_parent.end();) {
        if (remaining.contains(it->first)) {
          ++it;
        } else {
          const auto &relay_parent = it->first;
          _keeper_.emplace_back(it->second.per_session_state);
          state_by_relay_parent_to_check_[relay_parent] = std::move(it->second);
          relay_parent_depth_[relay_parent] = block_header.number;
          it = our_current_state_.state_by_relay_parent.erase(it);
        }
      }
    }

    for (auto it = our_current_state_.per_candidate.begin();
         it != our_current_state_.per_candidate.end();) {
      if (our_current_state_.state_by_relay_parent.contains(
              it->second.relay_parent)) {
        ++it;
      } else {
        it = our_current_state_.per_candidate.erase(it);
      }
    }

    std::vector<Hash> fresh_relay_parents;
    ProspectiveParachainsModeOpt leaf_mode;
    if (!res) {
      if (our_current_state_.per_leaf.contains(relay_parent)) {
        return pruned;
      }

      our_current_state_.per_leaf.insert_or_assign(relay_parent,
                                                   SecondedList{});
      fresh_relay_parents.emplace_back(relay_parent);
      leaf_mode = std::nullopt;
    } else if (res->has_value()) {
      const ActiveLeafState active_leaf_state = res->value();
      our_current_state_.per_leaf.insert_or_assign(relay_parent,
                                                   active_leaf_state);

      if (auto f = our_current_state_.implicit_view
                       ->known_allowed_relay_parents_under(relay_parent,
                                                           std::nullopt)) {
        fresh_relay_parents.insert(
            fresh_relay_parents.end(), f->begin(), f->end());
        leaf_mode = res->value();
      } else {
        SL_TRACE(logger_,
                 "Implicit view gave no relay-parents. (leaf_hash={})",
                 relay_parent);
        fresh_relay_parents.emplace_back(relay_parent);
        leaf_mode = res->value();
      }
    } else {
      SL_TRACE(
          logger_,
          "Failed to load implicit view for leaf. (leaf_hash={}, error={})",
          relay_parent,
          res->error());

      return pruned;
    }

    for (const auto &maybe_new : fresh_relay_parents) {
      if (our_current_state_.state_by_relay_parent.contains(maybe_new)) {
        continue;
      }

      ProspectiveParachainsModeOpt mode_;
      if (auto l = utils::get(our_current_state_.per_leaf, maybe_new)) {
        mode_ = from(l->get());
      } else {
        mode_ = leaf_mode;
      }

      auto rps_result = construct_per_relay_parent_state(maybe_new, mode_);
      if (rps_result.has_value()) {
        our_current_state_.state_by_relay_parent.insert_or_assign(
            maybe_new, std::move(rps_result.value()));
      } else if (rps_result.error() != Error::KEY_NOT_PRESENT) {
        SL_TRACE(
            logger_,
            "Relay parent state was not created. (relay parent={}, error={})",
            maybe_new,
            rps_result.error());
      }
    }

    return pruned;
  }

  void ParachainProcessorImpl::second_unblocked_collations(
      ParachainId para_id,
      const HeadData &head_data,
      const Hash &head_data_hash) {
    auto unblocked_collations_it =
        our_current_state_.validator_side.blocked_from_seconding.find(
            BlockedCollationId(para_id, head_data_hash));

    if (unblocked_collations_it
        != our_current_state_.validator_side.blocked_from_seconding.end()) {
      auto &unblocked_collations = unblocked_collations_it->second;

      if (!unblocked_collations.empty()) {
        SL_TRACE(logger_,
                 "Candidate outputting head data with hash {} unblocked {} "
                 "collations for seconding.",
                 head_data_hash,
                 unblocked_collations.size());
      }

      for (auto &unblocked_collation : unblocked_collations) {
        unblocked_collation.maybe_parent_head_data = head_data;
        const auto peer_id =
            unblocked_collation.collation_event.pending_collation.peer_id;
        const auto relay_parent =
            unblocked_collation.candidate_receipt.descriptor.relay_parent;

        if (auto res = kick_off_seconding(std::move(unblocked_collation));
            res.has_error()) {
          SL_WARN(logger_,
                  "Seconding aborted due to an error. (relay_parent={}, "
                  "para_id={}, peer_id={}, error={})",
                  relay_parent,
                  para_id,
                  peer_id,
                  res.error());
        }
      }

      our_current_state_.validator_side.blocked_from_seconding.erase(
          unblocked_collations_it);
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
              .candidate_receipt = value.receipt,
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
              .candidate_receipt = value.receipt,
              .pov = std::move(value.pov),
              .maybe_parent_head_data = std::move(value.parent_head_data),
          };
        });

    CHECK_OR_RET(p.has_value());
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
    } else if (res.value() == false) {
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
      std::optional<std::pair<std::reference_wrapper<const HeadData>,
                              std::reference_wrapper<const Hash>>>
          maybe_parent_head_and_hash) {
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
        && hasher_->blake2b_256(maybe_parent_head_and_hash->first.get())
               != maybe_parent_head_and_hash->second.get()) {
      return Error::PARENT_HEAD_DATA_MISMATCH;
    }

    return outcome::success();
  }

  void ParachainProcessorImpl::dequeue_next_collation_and_fetch(
      const RelayHash &relay_parent,
      std::pair<CollatorId, std::optional<CandidateHash>> previous_fetch) {
    TRY_GET_OR_RET(per_relay_state, tryGetStateByRelayParent(relay_parent));

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
    TRY_GET_OR_RET(parachain_state, tryGetStateByRelayParent(bd->relay_parent));

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

  void ParachainProcessorImpl::process_vstaging_statement(
      const libp2p::peer::PeerId &peer_id,
      const network::vstaging::StatementDistributionMessage &msg) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    SL_TRACE(
        logger_, "Incoming `StatementDistributionMessage`. (peer={})", peer_id);

    if (auto inner =
            if_type<const network::vstaging::BackedCandidateAcknowledgement>(
                msg)) {
      statement_distribution->handle_incoming_acknowledgement(peer_id,
                                                              inner->get());
    } else if (auto manifest =
                   if_type<const network::vstaging::BackedCandidateManifest>(
                       msg)) {
      statement_distribution->handle_incoming_manifest(peer_id,
                                                       manifest->get());
    } else if (auto stm =
                   if_type<const network::vstaging::
                               StatementDistributionMessageStatement>(msg)) {
      statement_distribution->handle_incoming_statement(peer_id, stm->get());
    } else {
      SL_ERROR(logger_, "Skipped message.");
    }
  }

  void ParachainProcessorImpl::process_legacy_statement(
      const libp2p::peer::PeerId &peer_id,
      const network::StatementDistributionMessage &msg) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    if (auto statement_msg{boost::get<const network::Seconded>(&msg)}) {
      CHECK_OR_RET(canProcessParachains().has_value());
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
  void ParachainProcessorImpl::requestPoV(const libp2p::peer::PeerId &peer_id,
                                          const CandidateHash &candidate_hash,
                                          F &&callback) {
    /// TODO(iceseer): request PoV from validator, who seconded candidate
    /// But now we can assume, that if we received either `seconded` or `valid`
    /// from some peer, than we expect this peer has valid PoV, which we can
    /// request.

    logger_->info(
        "Requesting PoV.(candidate hash={}, peer={})", candidate_hash, peer_id);

    auto protocol = router_->getReqPovProtocol();
    protocol->request(peer_id, candidate_hash, std::forward<F>(callback));
  }

  void ParachainProcessorImpl::kickOffValidationWork(
      const RelayHash &relay_parent,
      AttestingData &attesting_data,
      const runtime::PersistedValidationData &persisted_validation_data,
      RelayParentState &parachain_state) {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    const auto candidate_hash{attesting_data.candidate.hash(*hasher_)};
    CHECK_OR_RET(!parachain_state.issued_statements.contains(candidate_hash));

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
          peer->id,
          candidate_hash,
          [candidate{attesting_data.candidate},
           pvd{std::move(pvd)},
           candidate_hash,
           wself{weak_from_this()},
           relay_parent,
           peer_id{peer->id}](auto &&pov_response_result) mutable {
            TRY_GET_OR_RET(self, wself.lock());
            auto parachain_state = self->tryGetStateByRelayParent(relay_parent);
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
                candidate, std::move(*p), std::move(pvd), relay_parent);
          });
    } else {
      SL_WARN(logger_,
              "No audi for PoV request. (relay_parent={}, candidate_hash={})",
              relay_parent,
              candidate_hash);
    }
  }

  outcome::result<network::FetchChunkResponse>
  ParachainProcessorImpl::OnFetchChunkRequest(
      const network::FetchChunkRequest &request) {
    if (auto chunk =
            av_store_->getChunk(request.candidate, request.chunk_index)) {
      return network::Chunk{
          .data = chunk->chunk,
          .chunk_index = request.chunk_index,
          .proof = chunk->proof,
      };
    }
    return network::Empty{};
  }

  outcome::result<network::FetchChunkResponseObsolete>
  ParachainProcessorImpl::OnFetchChunkRequestObsolete(
      const network::FetchChunkRequest &request) {
    if (auto chunk =
            av_store_->getChunk(request.candidate, request.chunk_index)) {
      // This check needed because v1 protocol mustn't have chunk mapping
      // https://github.com/paritytech/polkadot-sdk/blob/d2fd53645654d3b8e12cbf735b67b93078d70113/polkadot/node/core/av-store/src/lib.rs#L1345
      if (chunk->index == request.chunk_index) {
        return network::ChunkObsolete{
            .data = chunk->chunk,
            .proof = chunk->proof,
        };
      }
    }
    return network::Empty{};
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
    REINVOKE(*main_pool_handler_, handleStatement, relay_parent, statement);
    TRY_GET_OR_RET(opt_parachain_state, tryGetStateByRelayParent(relay_parent));

    auto &parachain_state = opt_parachain_state->get();
    const auto &assigned_core = parachain_state.assigned_core;
    auto &fallbacks = parachain_state.fallbacks;
    auto &awaiting_validation = parachain_state.awaiting_validation;
    auto &table_context = parachain_state.table_context;

    auto res = importStatement(relay_parent, statement, parachain_state);
    if (res.has_error()) {
      SL_TRACE(logger_,
               "Statement rejected. (relay_parent={}, error={}).",
               relay_parent,
               res.error());
      return;
    }

    post_import_statement_actions(relay_parent, parachain_state, res.value());
    if (auto summary = res.value()) {
      const auto &candidate_hash = summary->candidate;
      if (!assigned_core || summary->group_id != *assigned_core) {
        return;
      }

      SL_TRACE(logger_,
               "Registered incoming statement. (relay_parent={}, "
               "candidate_hash={}).",
               relay_parent,
               candidate_hash);
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

                const auto &[it, _] = fallbacks.insert(
                    std::make_pair(candidate_hash, std::move(attesting)));
                return it->second;
              },
              [&](const StatementWithPVDValid &val)
                  -> std::optional<std::reference_wrapper<AttestingData>> {
                auto it = fallbacks.find(val.candidate_hash);
                if (it == fallbacks.end()) {
                  return std::nullopt;
                }

                const auto our_index = utils::map(
                    table_context.validator,
                    [](const auto &signer) { return signer.validatorIndex(); });
                if (our_index && *our_index == statement.payload.ix) {
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
        } else {
          SL_TRACE(logger_,
                   "Candidate not found.(relay_parent={}, candidate_hash={}).",
                   relay_parent,
                   candidate_hash);
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

  outcome::result<BlockNumber>
  ParachainProcessorImpl::get_block_number_under_construction(
      const RelayHash &relay_parent) const {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    OUTCOME_TRY(header, block_tree_->tryGetBlockHeader(relay_parent));
    if (not header) {
      return 0;
    }
    return header.value().number + 1;
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

    auto r = get_block_number_under_construction(relay_parent);
    if (r.has_error()) {
      return {};
    }
    BlockNumber block_number = r.value();

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

      std::unordered_set<CandidateHash> para_ancestors_vec(
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
          }
          with_validation_code = true;
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
                .kind = network::ValidityAttestation::Implicit{},
                .signature = ((ValidatorSignature &)val),
            };
          },
          [](const BackingStore::ValidityVoteValid &val) {
            return network::ValidityAttestation{
                .kind = network::ValidityAttestation::Explicit{},
                .signature = ((ValidatorSignature &)val),
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
    }

    SL_TRACE(logger_, "No candidate info. (relay_parent={})", relay_parent);
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
    std::ranges::sort(vote_positions, [](const auto &l, const auto &r) {
      return l.second < r.second;
    });

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
        candidateHashFrom(parachain::getPayload(statement), hasher_);

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
        if (!prospective_parachains_->introduce_seconded_candidate(
                candidate.descriptor.para_id,
                candidate,
                crypto::Hashed<runtime::PersistedValidationData,
                               32,
                               crypto::Blake2b_StreamHasher<32>>{
                    seconded->get().pvd},
                candidate_hash)) {
          return Error::REJECTED_BY_PROSPECTIVE_PARACHAINS;
        }
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

    auto core =
        core_index_from_statement(rp_state.validator_to_group,
                                  rp_state.group_rotation_info,
                                  uint32_t(rp_state.availability_cores.size()),
                                  statement,
                                  rp_state.claim_queue);
    if (!core) {
      return Error::CORE_INDEX_UNAVAILABLE;
    };

    return importStatementToTable(
        relay_parent, rp_state, *core, candidate_hash, stmnt);
  }

  std::optional<CoreIndex> ParachainProcessorImpl::core_index_from_statement(
      const std::vector<std::optional<GroupIndex>> &validator_to_group,
      const runtime::GroupDescriptor &group_rotation_info,
      uint32_t n_cores,
      const SignedFullStatementWithPVD &statement,
      const runtime::ClaimQueueSnapshot &claim_queue) {
    const auto &compact_statement = getPayload(statement);
    const auto candidate_hash = candidateHashFrom(compact_statement, hasher_);

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
      const auto assigned_paras = claim_queue.iter_claims_for_core(core_index);

      const bool any = (std::ranges::find(assigned_paras, candidate_para_id)
                        != assigned_paras.end());
      if (!any) {
        SL_DEBUG(logger_,
                 "Invalid CoreIndex, core is not assigned to this para_id. "
                 "(candidate_hash={}, core_index={}, para_id={})",
                 candidate_hash,
                 core_index,
                 candidate_para_id);
        return std::nullopt;
      }
      return core_index;
    }
    return core_index;
  }

  template <ParachainProcessorImpl::StatementType kStatementType>
  outcome::result<std::optional<SignedFullStatementWithPVD>>
  ParachainProcessorImpl::sign_import_and_distribute_statement(
      ParachainProcessorImpl::RelayParentState &rp_state,
      const ValidateAndSecondResult &validation_result) {
    if (auto statement =
            createAndSignStatement<kStatementType>(validation_result)) {
      metric_kagome_parachain_candidate_backing_signed_statements_total_->inc();
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
      statement_distribution->share_local_statement(
          validation_result.relay_parent, stm);

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
    CHECK_OR_RET(summary);
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
          SL_DEBUG(
              logger_,
              "Candidate backed.(candidate={}, para id={}, relay_parent={})",
              summary->candidate,
              summary->group_id,
              relay_parent);
          if (rp_state.prospective_parachains_mode) {
            prospective_parachains_->candidate_backed(para_id,
                                                      summary->candidate);
            statement_distribution->handle_backed_candidate_message(
                summary->candidate);
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

    if (!parachain_state->get().table_context.validator) {
      logger_->warn("We are not validators or we have no validator index.");
      return std::nullopt;
    }

    if constexpr (kStatementType == StatementType::kSeconded) {
      return createAndSignStatementFromPayload(
          network::Statement{
              network::CandidateState{network::CommittedCandidateReceipt{
                  .descriptor = validation_result.candidate.descriptor,
                  .commitments = *validation_result.commitments}}},
          parachain_state->get());
    } else if constexpr (kStatementType == StatementType::kValid) {
      return createAndSignStatementFromPayload(
          network::Statement{network::CandidateState{
              validation_result.candidate.hash(*hasher_)}},
          parachain_state->get());
    }
  }

  template <typename T>
  std::optional<network::SignedStatement>
  ParachainProcessorImpl::createAndSignStatementFromPayload(
      T &&payload, RelayParentState &parachain_state) {
    /// TODO(iceseer):
    /// https://github.com/paritytech/polkadot/blob/master/primitives/src/v2/mod.rs#L1535-L1545
    auto sign_result =
        parachain_state.table_context.validator->sign(std::forward<T>(payload));
    if (sign_result.has_error()) {
      logger_->error(
          "Unable to sign Commited Candidate Receipt. Failed with error: {}",
          sign_result.error());
      return std::nullopt;
    }

    return sign_result.value();
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

    router_->getCollationProtocol()->write(peer_id,
                                           network::Seconded{
                                               .relay_parent = relay_parent,
                                               .statement = std::move(stm),
                                           });
  }

  template <bool kReinvoke>
  void ParachainProcessorImpl::notifyInvalid(
      const primitives::BlockHash &parent,
      const network::CandidateReceipt &candidate_receipt) {
    REINVOKE_ONCE(
        *main_pool_handler_, notifyInvalid, parent, candidate_receipt);

    our_current_state_.validator_side.blocked_from_seconding.erase(
        BlockedCollationId(candidate_receipt.descriptor.para_id,
                           candidate_receipt.descriptor.para_head_hash));

    auto fetched_collation =
        network::FetchedCollation::from(candidate_receipt, *hasher_);
    const auto &candidate_hash = fetched_collation.candidate_hash;

    auto it = our_current_state_.validator_side.fetched_candidates.find(
        fetched_collation);
    CHECK_OR_RET(it
                 != our_current_state_.validator_side.fetched_candidates.end());

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
    /// https://github.com/qdrvm/kagome/issues/2134
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

    auto output_head_data =
        seconded->get().committed_receipt.commitments.para_head;
    auto output_head_data_hash =
        seconded->get().committed_receipt.descriptor.para_head_hash;
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
      if (peer_data->get().collation_version) {
        version = *peer_data->get().collation_version;
      }
      notify_collation_seconded(peer_id, version, relay_parent, statement);
    }

    if (auto rp_state = tryGetStateByRelayParent(parent)) {
      rp_state->get().collations.status = CollationStatus::Seconded;
      rp_state->get().collations.note_seconded();
    }

    second_unblocked_collations(
        fetched_collation.para_id, output_head_data, output_head_data_hash);

    const auto maybe_candidate_hash = utils::map(
        prospective_candidate, [](const auto &v) { return v.candidate_hash; });

    dequeue_next_collation_and_fetch(parent,
                                     {collator_id, maybe_candidate_hash});

    /// TODO(iceseer): Bump collator reputation
  }

  bool ParachainProcessorImpl::isValidatingNode() const {
    return app_config_.roles().isAuthority();
  }

  void ParachainProcessorImpl::onValidationComplete(
      const ValidateAndSecondResult &validation_result) {
    logger_->trace("On validation complete. (relay parent={})",
                   validation_result.relay_parent);

    TRY_GET_OR_RET(opt_parachain_state,
                   tryGetStateByRelayParent(validation_result.relay_parent));
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

    CHECK_OR_RET(!parachain_state.issued_statements.contains(candidate_hash));
    logger_->trace("Second candidate complete. (candidate={}, relay parent={})",
                   candidate_hash,
                   validation_result.relay_parent);

    metric_kagome_parachain_candidate_backing_candidates_seconded_total_->inc();

    const auto parent_head_data_hash =
        hasher_->blake2b_256(validation_result.pvd.parent_head);
    const auto ph =
        hasher_->blake2b_256(validation_result.commitments->para_head);
    CHECK_OR_RET(parent_head_data_hash != ph);

    HypotheticalCandidateComplete hypothetical_candidate{
        .candidate_hash = candidate_hash,
        .receipt =
            network::CommittedCandidateReceipt{
                .descriptor = validation_result.candidate.descriptor,
                .commitments = *validation_result.commitments,
            },
        .persisted_validation_data = validation_result.pvd,
    };

    TRY_GET_OR_RET(hypothetical_membership,
                   seconding_sanity_check(hypothetical_candidate));

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

    CHECK_OR_RET(res.value());
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

    for (const auto &leaf : *hypothetical_membership) {
      auto it = our_current_state_.per_leaf.find(leaf);
      if (it == our_current_state_.per_leaf.end()) {
        SL_WARN(logger_,
                "Missing `per_leaf` for known active leaf. (leaf={})",
                leaf);
        continue;
      }

      ActiveLeafState &leaf_data = it->second;
      add_seconded_candidate(leaf_data,
                             validation_result.candidate.descriptor.para_id);
    }

    parachain_state.issued_statements.insert(candidate_hash);
    notifySeconded(validation_result.relay_parent, stmt);
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

    TRY_GET_OR_RET(
        parachain_state,
        tryGetStateByRelayParent(validate_and_second_result.relay_parent));
    SL_INFO(logger_,
            "Async validation complete.(relay parent={}, para_id={})",
            validate_and_second_result.relay_parent,
            validate_and_second_result.candidate.descriptor.para_id);

    parachain_state->get().awaiting_validation.erase(candidate_hash);
    auto q{std::move(validate_and_second_result)};
    if constexpr (kMode == ValidationTaskType::kSecond) {
      onValidationComplete(q);
    } else {
      onAttestComplete(q);
    }
  }

  template <ParachainProcessorImpl::ValidationTaskType kMode>
  void ParachainProcessorImpl::validateAsync(
      network::CandidateReceipt candidate,
      network::ParachainBlock &&pov,
      runtime::PersistedValidationData &&pvd,
      const primitives::BlockHash &_relay_parent) {
    REINVOKE(*main_pool_handler_,
             validateAsync<kMode>,
             candidate,
             std::move(pov),
             std::move(pvd),
             _relay_parent);
    const auto relay_parent = candidate.descriptor.relay_parent;

    TRY_GET_OR_RET(parachain_state,
                   tryGetStateByRelayParent(candidate.descriptor.relay_parent));
    const auto candidate_hash{candidate.hash(*hasher_)};
    if constexpr (kMode == ValidationTaskType::kAttest) {
      CHECK_OR_RET(
          !parachain_state->get().issued_statements.contains(candidate_hash));
    }

    CHECK_OR_RET(parachain_state->get()
                     .awaiting_validation.insert(candidate_hash)
                     .second);
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
      TRY_GET_OR_RET(self, weak_self.lock());
      if (!validation_result) {
        SL_WARN(self->logger_,
                "Candidate {} on relay_parent {}, para_id {} validation failed "
                "with "
                "error: {}",
                candidate_hash,
                candidate.descriptor.relay_parent,
                candidate.descriptor.para_id,
                validation_result.error());
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
                TRY_GET_OR_RET(self, weak_self.lock());
                post(*self->main_pool_handler_,
                     [cb{std::move(cb)}, r{std::move(r)}]() mutable {
                       cb(std::move(r));
                     });
              });
  }

  void ParachainProcessorImpl::onAttestComplete(
      const ValidateAndSecondResult &result) {
    TRY_GET_OR_RET(parachain_state,
                   tryGetStateByRelayParent(result.relay_parent));
    SL_INFO(logger_,
            "Attest complete.(relay parent={}, para id={})",
            result.relay_parent,
            result.candidate.descriptor.para_id);

    const auto candidate_hash = result.candidate.hash(*hasher_);
    parachain_state->get().fallbacks.erase(candidate_hash);

    if (!parachain_state->get().issued_statements.contains(candidate_hash)) {
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
    TRY_GET_OR_RET(parachain_state, tryGetStateByRelayParent(relay_parent));

    auto it = parachain_state->get().fallbacks.find(candidate_hash);
    CHECK_OR_RET(it != parachain_state->get().fallbacks.end());

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
      return active_leaves.contains(relay_parent);
    }

    for (const auto &[hash, mode] : active_leaves) {
      if (mode) {
        if (const auto k = implicit_view.known_allowed_relay_parents_under(
                hash, para_id)) {
          for (const auto &h : *k) {
            if (h == relay_parent) {
              return true;
            }
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

    if (!isRelayParentInImplicitView(
            on_relay_parent,
            relay_parent_mode,
            *our_current_state_.implicit_view,
            our_current_state_.validator_side.active_leaves,
            peer_data.collator_state->para_id)) {
      SL_TRACE(logger_, "Out of view. (relay_parent={})", on_relay_parent);
      return Error::OUT_OF_VIEW;
    }

    if (!relay_parent_mode) {
      if (peer_data.collator_state->advertisements.contains(on_relay_parent)) {
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

  // Attempt to kick off the seconding process for a pending collation
  outcome::result<bool> ParachainProcessorImpl::kick_off_seconding(
      network::PendingCollationFetch &&pending_collation_fetch) {
    // Ensure this function is running in the main thread
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());

    // Extract necessary data from the pending collation fetch
    auto &collation_event = pending_collation_fetch.collation_event;
    auto pending_collation = collation_event.pending_collation;
    auto relay_parent = pending_collation.relay_parent;

    // Retrieve the state associated with the relay parent
    OUTCOME_TRY(per_relay_parent, getStateByRelayParent(relay_parent));

    // Perform a sanity check on the descriptor version
    OUTCOME_TRY(descriptorVersionSanityCheck(
        pending_collation_fetch.candidate_receipt.descriptor,
        per_relay_parent.get().v2_receipts,
        per_relay_parent.get().current_core,
        per_relay_parent.get().per_session_state->value().session));

    // Check if the collation has already been fetched
    auto &collations = per_relay_parent.get().collations;
    auto fetched_collation = network::FetchedCollation::from(
        pending_collation_fetch.candidate_receipt, *hasher_);

    if (our_current_state_.validator_side.fetched_candidates.contains(
            fetched_collation)) {
      return Error::DUPLICATE;
    }

    // Set the commitments hash for the pending collation
    collation_event.pending_collation.commitments_hash =
        pending_collation_fetch.candidate_receipt.commitments_hash;

    // Determine the collation version and prospective candidate status
    const bool is_collator_v2 = (collation_event.collator_protocol_version
                                 == network::CollationVersion::VStaging);
    const bool have_prospective_candidate =
        collation_event.pending_collation.prospective_candidate.has_value();
    const bool async_backing_en =
        per_relay_parent.get().prospective_parachains_mode.has_value();

    // Initialize optional variables for validation data and parent head hash
    std::optional<runtime::PersistedValidationData> maybe_pvd;
    std::optional<Hash> maybe_parent_head_hash;
    std::optional<HeadData> &maybe_parent_head =
        pending_collation_fetch.maybe_parent_head_data;

    // Fetch prospective validation data if applicable
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
        maybe_parent_head_hash.emplace(
            collation_event.pending_collation.prospective_candidate
                ->parent_head_data_hash);
      }
    } else if ((is_collator_v2 && have_prospective_candidate)
               || !is_collator_v2) {
      // Fetch persisted validation data if applicable
      OUTCOME_TRY(
          pvd,
          requestPersistedValidationData(
              pending_collation_fetch.candidate_receipt.descriptor.relay_parent,
              pending_collation_fetch.candidate_receipt.descriptor.para_id));
      maybe_pvd = pvd;
      maybe_parent_head_hash = std::nullopt;
    } else {
      return outcome::success(false);
    }

    // Handle cases where validation data is not found
    std::optional<std::reference_wrapper<runtime::PersistedValidationData>> pvd;
    if (maybe_pvd) {
      pvd = *maybe_pvd;
    } else if (!maybe_pvd && !maybe_parent_head && maybe_parent_head_hash) {
      const network::PendingCollationFetch blocked_collation{
          .collation_event = std::move(collation_event),
          .candidate_receipt = pending_collation_fetch.candidate_receipt,
          .pov = std::move(pending_collation_fetch.pov),
          .maybe_parent_head_data = std::nullopt,
      };
      SL_TRACE(logger_,
               "Collation having parent head data hash {} is blocked from "
               "seconding. Waiting on its parent to be validated. "
               "(candidate_hash={}, relay_parent={})",
               *maybe_parent_head_hash,
               blocked_collation.candidate_receipt.hash(*hasher_),
               blocked_collation.candidate_receipt.descriptor.relay_parent);
      our_current_state_.validator_side
          .blocked_from_seconding[BlockedCollationId(
              blocked_collation.candidate_receipt.descriptor.para_id,
              *maybe_parent_head_hash)]
          .emplace_back(blocked_collation);

      return outcome::success(false);
    } else {
      return Error::PERSISTED_VALIDATION_DATA_NOT_FOUND;
    }

    // Perform a sanity check on the fetched collation
    OUTCOME_TRY(fetched_collation_sanity_check(
        collation_event.pending_collation,
        pending_collation_fetch.candidate_receipt,
        pvd->get(),
        maybe_parent_head && maybe_parent_head_hash
            ? std::make_pair(std::cref(*maybe_parent_head),
                             std::cref(*maybe_parent_head_hash))
            : std::optional<std::pair<std::reference_wrapper<const HeadData>,
                                      std::reference_wrapper<const Hash>>>{}));

    // Retrieve the state associated with the relay parent again
    OUTCOME_TRY(
        rp_state,
        getStateByRelayParent(
            pending_collation_fetch.candidate_receipt.descriptor.relay_parent));
    std::optional<std::reference_wrapper<const std::vector<ParachainId>>>
        assigned_paras;
    if (rp_state.get().assigned_core) {
      const auto &claimes = rp_state.get().claim_queue.claimes;
      auto it = claimes.find(*rp_state.get().assigned_core);
      if (it != claimes.end()) {
        assigned_paras = std::cref(it->second);
      }
    }

    // Check if the para ID is within the assigned paras
    if (!assigned_paras
        || std::ranges::find(
               assigned_paras->get(),
               pending_collation_fetch.candidate_receipt.descriptor.para_id)
               == assigned_paras->get().end()) {
      SL_INFO(
          logger_,
          "Subsystem asked to second for para outside of our assignment.(para "
          "id={}, "
          "relay parent={})",
          pending_collation_fetch.candidate_receipt.descriptor.para_id,
          pending_collation_fetch.candidate_receipt.descriptor.relay_parent);
      return outcome::success(false);
    }

    // Set the collation status to waiting on validation and start async
    // validation
    collations.status = CollationStatus::WaitingOnValidation;
    validateAsync<ValidationTaskType::kSecond>(
        pending_collation_fetch.candidate_receipt,
        std::move(pending_collation_fetch.pov),
        std::move(pvd->get()),
        relay_parent);

    // Store the fetched collation in the current state
    our_current_state_.validator_side.fetched_candidates.emplace(
        fetched_collation, collation_event);
    return outcome::success(true);
  }

  ParachainProcessorImpl::SecondingAllowed
  ParachainProcessorImpl::seconding_sanity_check(
      const HypotheticalCandidate &hypothetical_candidate) {
    const auto &active_leaves = our_current_state_.per_leaf;
    const auto &implicit_view = *our_current_state_.implicit_view;

    std::vector<Hash> leaves_for_seconding;
    const auto candidate_para = candidatePara(hypothetical_candidate);
    const auto candidate_relay_parent = relayParent(hypothetical_candidate);
    const auto candidate_hash = candidateHash(hypothetical_candidate);

    auto proc_response = [&](bool is_member_or_potential, const Hash &head) {
      if (!is_member_or_potential) {
        SL_TRACE(logger_,
                 "Refusing to second candidate at leaf. Is not a potential "
                 "member. (candidate_hash={}, leaf_hash={})",
                 candidate_hash.get(),
                 head);
      } else {
        leaves_for_seconding.emplace_back(head);
      }
    };

    for (const auto &[head, leaf_state] : active_leaves) {
      if (is_type<ProspectiveParachainsMode>(leaf_state)) {
        const auto allowed_parents_for_para =
            implicit_view.known_allowed_relay_parents_under(
                head, {candidate_para.get()});
        if (!allowed_parents_for_para
            || std::find(allowed_parents_for_para->begin(),
                         allowed_parents_for_para->end(),
                         candidate_relay_parent.get())
                   == allowed_parents_for_para->end()) {
          continue;
        }

        bool is_member_or_potential = false;
        for (auto &&[candidate, leaves] :
             prospective_parachains_->answer_hypothetical_membership_request(
                 std::span<const HypotheticalCandidate>{&hypothetical_candidate,
                                                        1},
                 {{head}})) {
          if (candidateHash(candidate).get() != candidate_hash.get()) {
            continue;
          }

          for (const auto &leaf : leaves) {
            if (leaf == head) {
              is_member_or_potential = true;
              break;
            }
          }

          if (is_member_or_potential) {
            break;
          }
        }

        proc_response(is_member_or_potential, head);
      } else {
        if (head == candidate_relay_parent.get()) {
          if (auto seconded = if_type<const SecondedList>(leaf_state)) {
            if (seconded->get().contains(candidate_para.get())) {
              return std::nullopt;
            }
          }

          proc_response(true, head);
        }
      }
    }

    if (leaves_for_seconding.empty()) {
      return std::nullopt;
    }

    return leaves_for_seconding;
  }

  bool ParachainProcessorImpl::canSecond(ParachainId candidate_para_id,
                                         const Hash &relay_parent,
                                         const CandidateHash &candidate_hash,
                                         const Hash &parent_head_data_hash) {
    auto per_relay_parent = tryGetStateByRelayParent(relay_parent);
    if (per_relay_parent) {
      if (per_relay_parent->get().prospective_parachains_mode) {
        if (auto seconding_allowed =
                seconding_sanity_check(HypotheticalCandidateIncomplete{
                    .candidate_hash = candidate_hash,
                    .candidate_para = candidate_para_id,
                    .parent_head_data_hash = parent_head_data_hash,
                    .candidate_relay_parent = relay_parent})) {
          return !seconding_allowed->empty();
        }
      }
    }
    return false;
  }

  void ParachainProcessorImpl::printStoragesLoad() {
    SL_TRACE(logger_,
             "[Parachain storages statistics]:"
             "\n\t-> state_by_relay_parent={}"
             "\n\t-> per_leaf={}"
             "\n\t-> per_candidate={}"
             "\n\t-> active_leaves={}"
             "\n\t-> collation_requests_cancel_handles={}"
             "\n\t-> validator_side.fetched_candidates={}",
             our_current_state_.state_by_relay_parent.size(),
             our_current_state_.per_leaf.size(),
             our_current_state_.per_candidate.size(),
             our_current_state_.validator_side.active_leaves.size(),
             our_current_state_.collation_requests_cancel_handles.size(),
             our_current_state_.validator_side.fetched_candidates.size());
    if (our_current_state_.implicit_view) {
      our_current_state_.implicit_view->printStoragesLoad();
    }
    prospective_parachains_->printStoragesLoad();
    bitfield_store_->printStoragesLoad();
    backing_store_->printStoragesLoad();
    av_store_->printStoragesLoad();
  }

  void ParachainProcessorImpl::handle_advertisement(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      std::optional<std::pair<CandidateHash, Hash>> &&prospective_candidate) {
    REINVOKE(*main_pool_handler_,
             handle_advertisement,
             relay_parent,
             peer_id,
             std::move(prospective_candidate));

    TRY_GET_OR_RET(opt_per_relay_parent,
                   tryGetStateByRelayParent(relay_parent));
    auto &per_relay_parent = opt_per_relay_parent->get();
    const ProspectiveParachainsModeOpt &relay_parent_mode =
        per_relay_parent.prospective_parachains_mode;

    TRY_GET_OR_RET(peer_state, pm_->getPeerState(peer_id));
    TRY_GET_OR_RET(collator_state, peer_state->get().collator_state);

    const ParachainId collator_para_id = collator_state->para_id;
    if (!per_relay_parent.assigned_core) {
      SL_TRACE(logger_,
               "We are not assigned. (peerd_id={}, collator={})",
               peer_id,
               collator_para_id);
      return;
    }

    const auto assigned_paras =
        per_relay_parent.claim_queue.iter_claims_for_core(
            *per_relay_parent.assigned_core);
    const bool any = (std::ranges::find(assigned_paras, collator_para_id)
                      != assigned_paras.end());

    if (!any) {
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
        .prospective_candidate = pc,
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
        std::ignore = fetchCollation(pending_collation, collator_id);
      } break;
      case CollationStatus::Seconded: {
        if (relay_parent_mode) {
          // Limit is not reached, it's allowed to second another
          // collation.
          std::ignore = fetchCollation(pending_collation, collator_id);
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
    if (peer_state->get().collation_version) {
      version = *peer_state->get().collation_version;
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
    if (our_current_state_.collation_requests_cancel_handles.contains(pc)) {
      SL_WARN(logger_,
              "Already requested. (relay parent={}, para id={})",
              pc.relay_parent,
              pc.para_id);
      return Error::ALREADY_REQUESTED;
    }

    OUTCOME_TRY(per_relay_parent, getStateByRelayParent(pc.relay_parent));
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

    our_current_state_.collation_requests_cancel_handles.insert(pc);
    const auto maybe_candidate_hash =
        utils::map(pc.prospective_candidate,
                   [](const auto &v) { return std::cref(v.candidate_hash); });
    per_relay_parent.get().collations.status = CollationStatus::Fetching;
    per_relay_parent.get().collations.fetching_from.emplace(
        id, maybe_candidate_hash);

    if (network::CollationVersion::V1 == version) {
      network::CollationFetchingRequest fetch_collation_request{
          .relay_parent = pc.relay_parent,
          .para_id = pc.para_id,
      };
      router_->getReqCollationProtocol()->request(
          peer_id, fetch_collation_request, std::move(response_callback));
    } else if (network::CollationVersion::VStaging == version
               && maybe_candidate_hash) {
      network::vstaging::CollationFetchingRequest fetch_collation_request{
          .relay_parent = pc.relay_parent,
          .para_id = pc.para_id,
          .candidate_hash = maybe_candidate_hash->get(),
      };
      router_->getReqCollationProtocol()->request(
          peer_id, fetch_collation_request, std::move(response_callback));
    } else {
      UNREACHABLE;
    }
    return outcome::success();
  }

  void ParachainProcessorImpl::onFinalize() {
    BOOST_ASSERT(main_pool_handler_->isInCurrentThread());
    if (not isValidatingNode()) {
      return;
    }
    if (state_by_relay_parent_to_check_.empty()) {
      return;
    }
    const auto last_finalized_block = block_tree_->getLastFinalized().number;
    static std::optional<primitives::BlockNumber>
        previous_last_finalized_block = std::nullopt;
    primitives::BlockNumber current_block_number = 0;
    if (not previous_last_finalized_block) {
      previous_last_finalized_block = last_finalized_block;
      if (last_finalized_block == 0) {
        return;
      }
    } else {
      current_block_number = previous_last_finalized_block.value() + 1;
    }
    for (auto i = current_block_number - 1; i < last_finalized_block; ++i) {
      const auto block_hash_res = block_tree_->getBlockHash(i);
      if (not block_hash_res) {
        SL_DEBUG(logger_,
                 "Error {} getting block hash for block number {}",
                 block_hash_res.error(),
                 i);
        continue;
      }
      const auto &block_hash_opt = block_hash_res.value();
      if (not block_hash_opt) {
        continue;
      }
      const auto &block_hash = block_hash_opt.value();
      const auto session_index =
          parachain_host_->session_index_for_child(block_hash);
      if (not session_index) {
        SL_DEBUG(logger_,
                 "Error {} getting session index for block {}",
                 session_index.error(),
                 block_hash);
      }
      metric_session_index_->set(session_index.value());
      proceedVotesOnRelayParent(block_hash);
    }
    previous_last_finalized_block = last_finalized_block;
    for (auto it = relay_parent_depth_.begin();
         it != relay_parent_depth_.end();) {
      if (it->second < last_finalized_block) {
        const auto &relay_parent = it->first;
        const auto jit = state_by_relay_parent_to_check_.find(relay_parent);
        if (jit != state_by_relay_parent_to_check_.end()) {
          proceedVotesOnRelayParent(relay_parent);
        }
        it = relay_parent_depth_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void ParachainProcessorImpl::proceedVotesOnRelayParent(
      const primitives::BlockHash &block_hash) {
    const auto it = state_by_relay_parent_to_check_.find(block_hash);
    if (it == state_by_relay_parent_to_check_.end()) {
      return;
    }

    struct CleanupGuard {
      std::function<void()> cleanup;
      ~CleanupGuard() {
        cleanup();
      }
    } cleanup_guard{[this, &block_hash] {
      state_by_relay_parent_to_check_.erase(block_hash);
    }};

    const auto &parachain_state = it->second;

    if (not parachain_state.assigned_core) {
      return;
    }

    const auto assigned_core = parachain_state.assigned_core.value();
    const auto group_it = parachain_state.table_context.groups.find(assigned_core);
    if (group_it == parachain_state.table_context.groups.end()) {
      return;
    }

    const auto validator_index_res =
        utils::map(parachain_state.table_context.validator,
                   [](const auto &signer) { return signer.validatorIndex(); });
    if (not validator_index_res) {
      return;
    }

    const auto validator_index = validator_index_res.value();
    const auto &group = group_it->second;
    std::unordered_map<ValidatorIndex, std::size_t> group_validator_position;

    for (std::size_t pos = 0; pos < group.size(); ++pos) {
      group_validator_position.emplace(group[pos], pos);
    }

    const auto validator_position_it = group_validator_position.find(validator_index);
    if (validator_position_it == group_validator_position.end()) {
      return;
    }

    const auto validator_position = validator_position_it->second;

    const auto availability_cores_res =
        parachain_host_->availability_cores(block_hash);
    if (not availability_cores_res) {
      SL_DEBUG(logger_,
               "Availability cores error {} on relay parent {}",
               availability_cores_res.error(),
               block_hash);
      return;
    }

    const auto &availability_cores = availability_cores_res.value();
    if (assigned_core >= availability_cores.size()) {
      return;
    }

    const auto parachain_id_opt = extractParachainId(availability_cores[assigned_core]);
    if (not parachain_id_opt) {
      return;
    }
    const auto &parachain_id = parachain_id_opt.value();
    const auto candidate_res = parachain_host_->candidate_pending_availability(
        block_hash, parachain_id);
    if (not candidate_res) {
      SL_DEBUG(logger_,
               "Candidate pending availability error {} on relay parent {}",
               candidate_res.error(),
               block_hash);
      return;
    }
    const auto &candidate_opt = candidate_res.value();
    if (not candidate_opt) {
      return;
    }

    const auto block_body_res = block_tree_->getBlockBody(block_hash);
    if (not block_body_res) {
      SL_DEBUG(logger_,
               "Block body error {} for block {}",
               block_body_res.error(),
               block_hash);
      return;
    }

    const auto &block_body = block_body_res.value();
    const auto parachain_inherent_data = extractParachainInherentData(block_body);
    if (not parachain_inherent_data) {
      return;
    }

    bool explicit_found = false, implicit_found = false;
    checkCandidateVotes(parachain_inherent_data.value(),
                        candidate_opt.value(),
                        validator_position,
                        explicit_found,
                        implicit_found);

    if (explicit_found) {
      SL_TRACE(logger_,
               "Explicit vote found for parachain {} on relay parent {}",
               parachain_id,
               block_hash);
      metric_kagome_parachain_candidate_explicit_votes_total_->inc();
    } else if (implicit_found) {
      SL_TRACE(logger_,
               "Implicit vote found for parachain {} on relay parent {}",
               parachain_id,
               block_hash);
      metric_kagome_parachain_candidate_implicit_votes_total_->inc();
    } else {
      SL_TRACE(logger_,
               "No vote found for parachain {} on relay parent {}",
               parachain_id,
               block_hash);
      metric_kagome_parachain_candidate_no_votes_total_->inc();
    }
  }

  std::optional<ParachainId> ParachainProcessorImpl::extractParachainId(
      const runtime::CoreState &core) const {
    if (auto occupied_core = std::get_if<runtime::OccupiedCore>(&core)) {
      return occupied_core->candidate_descriptor.para_id;
    }
    return std::nullopt;
  }

  std::optional<parachain::ParachainInherentData>
  ParachainProcessorImpl::extractParachainInherentData(
      const std::vector<primitives::Extrinsic> &block_body) const {
    for (const auto &extrinsic : block_body) {
      if (extrinsic.data.size() < 3
          || extrinsic.data[0] != parachain_inherent_data_extrinsic_version
          || extrinsic.data[1] != parachain_inherent_data_call
          || extrinsic.data[2] != parachain_inherent_data_module) {
        continue;
      }

      std::vector<uint8_t> buffer(extrinsic.data.begin() + 3,
                                  extrinsic.data.end());
      auto decode_res = scale::decode<parachain::ParachainInherentData>(buffer);

      if (decode_res) {
        return decode_res.value();
      }

      SL_DEBUG(logger_,
               "Failed to decode ParachainInherentData: {}",
               decode_res.error());
    }

    return std::nullopt;
  }

  void ParachainProcessorImpl::checkCandidateVotes(
      const parachain::ParachainInherentData &parachain_inherent_data,
      const runtime::CommittedCandidateReceipt &candidate,
      std::size_t validator_position,
      bool &explicit_found,
      bool &implicit_found) const {
    for (const auto &backed_candidate :
         parachain_inherent_data.backed_candidates) {
      if (backed_candidate.candidate != candidate) {
        continue;
      }

      if (backed_candidate.validator_indices.bits.size() <= validator_position
          || backed_candidate.validity_votes.size() <= validator_position
          || not backed_candidate.validator_indices.bits[validator_position]) {
        return;
      }

      boost::apply_visitor(
          [&](const auto &attestation) {
            using T = std::decay_t<decltype(attestation)>;
            if constexpr (std::is_same_v<
                              T,
                              network::ValidityAttestation::Implicit>) {
              implicit_found = true;
            } else if constexpr (std::is_same_v<
                                     T,
                                     network::ValidityAttestation::Explicit>) {
              explicit_found = true;
            }
          },
          backed_candidate.validity_votes[validator_position].kind);

      break;
    }
  }

}  // namespace kagome::parachain

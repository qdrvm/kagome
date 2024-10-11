/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <parachain/validator/statement_distribution/statement_distribution.hpp>

namespace kagome::parachain::statement_distribution {

    StatementDistribution::StatementDistribution(
        std::shared_ptr<parachain::ValidatorSignerFactory> sf,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        StatementDistributionThreadPool &statements_distribution_thread_pool)
        : per_session(RefCache<SessionIndex, PerSessionState>::create()),
          signer_factory(std::move(sf)),
          statements_distribution_thread_handler(poolHandlerReadyMake(
              this, app_state_manager, statements_distribution_thread_pool, logger)) {}


              /*
    remote_view_sub_ = std::make_shared<network::PeerView::PeerViewSubscriber>(
        peer_view_->getRemoteViewObservable(), false);
    primitives::events::subscribe(
        *remote_view_sub_,
        network::PeerView::EventType::kViewUpdated,
        [wptr{weak_from_this()}](const libp2p::peer::PeerId &peer_id,
                                 const network::View &view) {
          TRY_GET_OR_RET(self, wptr.lock());
          self->handle_peer_view_update(peer_id, view);
        });
              
              */

  void StatementDistribution::handle_peer_view_update(
      const libp2p::peer::PeerId &peer, const network::View &new_view) {
    REINVOKE(*main_pool_handler_, handle_peer_view_update, peer, new_view);
    TRY_GET_OR_RET(peer_state, pm_->getPeerState(peer));

    auto fresh_implicit = peer_state->get().update_view(
        new_view, *our_current_state_.implicit_view);
    for (const auto &new_relay_parent : fresh_implicit) {
      send_peer_messages_for_relay_parent(peer, new_relay_parent);
    }
  }

  void StatementDistribution::circulate_statement(
      const RelayHash &relay_parent,
      RelayParentState &relay_parent_state,
      const IndexedAndSigned<network::vstaging::CompactStatement> &statement) {
    const auto &session_info =
        relay_parent_state.per_session_state->value().session_info;
    const auto &compact_statement = getPayload(statement);
    const auto &candidate_hash = candidateHash(compact_statement);
    const auto originator = statement.payload.ix;
    const auto is_confirmed = candidates_.is_confirmed(candidate_hash);

    CHECK_OR_RET(relay_parent_state.local_validator);
    enum DirectTargetKind : uint8_t {
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
        const auto can_use_grid = !cluster_relevant
                               || std::ranges::find(all_cluster_targets, v)
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
      if (peer_state->get().collation_version) {
        version = *peer_state->get().collation_version;
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


  void StatementDistribution::share_local_statement(
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


  void StatementDistribution::send_pending_grid_messages(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      ValidatorIndex peer_validator_id,
      const Groups &groups,
      PerRelayParentState &relay_parent_state) {
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

  void StatementDistribution::send_pending_cluster_statements(
      const RelayHash &relay_parent,
      const libp2p::peer::PeerId &peer_id,
      network::CollationVersion version,
      ValidatorIndex peer_validator_id,
      PerRelayParentState &relay_parent_state) {
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


  std::optional<std::pair<std::vector<libp2p::peer::PeerId>,
                          network::VersionedValidatorProtocolMessage>>
  StatementDistribution::pending_statement_network_message(
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


  void StatementDistribution::send_peer_messages_for_relay_parent(
      const libp2p::peer::PeerId &peer_id, const RelayHash &relay_parent) {
    BOOST_ASSERT(
        main_pool_handler_
            ->isInCurrentThread());  // because of pm_->getPeerState(...)

    //TRY_GET_OR_RET(peer_state, pm_->getPeerState(peer_id));
    TRY_GET_OR_RET(parachain_state, tryGetStateByRelayParent(relay_parent));

    network::CollationVersion version = network::CollationVersion::VStaging;
//    if (peer_state->get().collation_version) {
//      version = *peer_state->get().collation_version;
//    }

    if (auto auth_id = query_audi_->get(peer_id)) {
      if (auto it = parachain_state->get().authority_lookup.find(*auth_id);
          it != parachain_state->get().authority_lookup.end()) {
        ValidatorIndex vi = it->second;

        SL_TRACE(logger_,
                 "Send pending cluster/grid messages. (peer={}. validator "
                 "index={}, relay_parent={})",
                 peer_id,
                 vi,
                 relay_parent);
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

  std::optional<LocalValidatorState>
  StatementDistribution::find_active_validator_state(
      ValidatorIndex validator_index,
      const Groups &groups,
      const std::vector<runtime::CoreState> &availability_cores,
      const runtime::GroupDescriptor &group_rotation_info,
      const std::optional<runtime::ClaimQueueSnapshot> &maybe_claim_queue,
      size_t seconding_limit,
      size_t max_candidate_depth) {
    BOOST_ASSERT(statements_distribution_thread_handler->isInCurrentThread());

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

}

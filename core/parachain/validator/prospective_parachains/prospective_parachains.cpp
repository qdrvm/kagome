/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include "utils/stringify.hpp"

#define COMPONENT ProspectiveParachains
#define COMPONENT_NAME STRINGIFY(COMPONENT)

namespace kagome::parachain {
  ProspectiveParachains::ProspectiveParachains(
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::ParachainHost> parachain_host,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : hasher_{std::move(hasher)},
        parachain_host_{std::move(parachain_host)},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(parachain_host_);
    BOOST_ASSERT(block_tree_);
  }

  void ProspectiveParachains::printStoragesLoad() {
    SL_TRACE(logger,
             "[Prospective parachains storages statistics]:"
             "\n\t-> view.active_leaves={}"
             "\n\t-> view.candidate_storage={}",
             view.active_leaves.size(),
             view.candidate_storage.size());
  }

  std::shared_ptr<blockchain::BlockTree> ProspectiveParachains::getBlockTree() {
    BOOST_ASSERT(block_tree_);
    return block_tree_;
  }

  std::vector<std::pair<ParachainId, BlockNumber>>
  ProspectiveParachains::answerMinimumRelayParentsRequest(
      const RelayHash &relay_parent) const {
    std::vector<std::pair<ParachainId, BlockNumber>> v;
    if (view.active_leaves.contains(relay_parent)) {
      if (auto leaf_data = utils::get(view.per_relay_parent, relay_parent)) {
      }
      const RelayBlockViewData &leaf_data = it->second;
      SL_TRACE(logger,
               "Found active list. (relay_parent={}, fragment_trees_count={})",
               relay_parent,
               leaf_data.fragment_trees.size());

      for (const auto &[para_id, fragment_tree] : leaf_data.fragment_trees) {
        v.emplace_back(para_id,
                       fragment_tree.scope.earliestRelayParent().number);
      }
    }
    return v;
  }

  std::vector<std::pair<CandidateHash, Hash>>
  ProspectiveParachains::answerGetBackableCandidates(
      const RelayHash &relay_parent,
      ParachainId para,
      uint32_t count,
      const std::vector<CandidateHash> &required_path) {
    SL_TRACE(logger,
             "Search for backable candidates. (para_id={}, "
             "relay_parent={})",
             para,
             relay_parent);
    auto data_it = view.active_leaves.find(relay_parent);
    if (data_it == view.active_leaves.end()) {
      SL_TRACE(logger,
               "Requested backable candidate for inactive relay-parent. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
      return {};
    }
    const RelayBlockViewData &data = data_it->second;

    auto tree_it = data.fragment_trees.find(para);
    if (tree_it == data.fragment_trees.end()) {
      SL_TRACE(logger,
               "Requested backable candidate for inactive para. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
      return {};
    }
    const fragment::FragmentTree &tree = tree_it->second;

    auto storage_it = view.candidate_storage.find(para);
    if (storage_it == view.candidate_storage.end()) {
      SL_WARN(logger,
              "No candidate storage for active para. (relay_parent={}, "
              "para_id={})",
              relay_parent,
              para);
      return {};
    }
    const fragment::CandidateStorage &storage = storage_it->second;

    std::vector<std::pair<CandidateHash, Hash>> backable_candidates;
    const auto children = tree.selectChildren(
        required_path, count, [&](const CandidateHash &candidate) -> bool {
          return storage.isBacked(candidate);
        });
    for (const auto &child_hash : children) {
      if (auto parent_hash_opt =
              storage.relayParentByCandidateHash(child_hash)) {
        backable_candidates.emplace_back(child_hash, *parent_hash_opt);
      } else {
        SL_ERROR(logger,
                 "Candidate is present in fragment tree but not in "
                 "candidate's storage! (child_hash={}, para_id={})",
                 child_hash,
                 para);
      }
    }

    if (backable_candidates.empty()) {
      SL_TRACE(logger,
               "Could not find any backable candidate. (relay_parent={}, "
               "para_id={})",
               relay_parent,
               para);
    } else {
      SL_TRACE(logger,
               "Found backable candidates. (relay_parent={}, count={})",
               relay_parent,
               backable_candidates.size());
    }

    return backable_candidates;
  }

  fragment::FragmentTreeMembership
  ProspectiveParachains::answerTreeMembershipRequest(
      ParachainId para, const CandidateHash &candidate) {
    SL_TRACE(logger,
             "Answer tree membership request. "
             "(para_id={}, candidate_hash={})",
             para,
             candidate);
    return fragmentTreeMembership(view.active_leaves, para, candidate);
  }

  std::optional<ProspectiveParachainsMode>
  ProspectiveParachains::prospectiveParachainsMode(
      const RelayHash &relay_parent) {
    auto result = parachain_host_->staging_async_backing_params(relay_parent);
    if (result.has_error()) {
      SL_TRACE(logger,
               "Prospective parachains are disabled, is not supported by the "
               "current Runtime API. (relay parent={}, error={})",
               relay_parent,
               result.error());
      return std::nullopt;
    }

    const parachain::fragment::AsyncBackingParams &vs = result.value();
    return ProspectiveParachainsMode{
        .max_candidate_depth = vs.max_candidate_depth,
        .allowed_ancestry_len = vs.allowed_ancestry_len,
    };
  }

  outcome::result<std::optional<fragment::RelayChainBlockInfo>>
  ProspectiveParachains::fetchBlockInfo(const RelayHash &relay_hash) {
    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// cache for block header request and calculations
    auto res_header = block_tree_->getBlockHeader(relay_hash);
    if (res_header.has_error()) {
      if (res_header.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
        return outcome::success(std::nullopt);
      }
      return res_header.error();
    }

    return fragment::RelayChainBlockInfo{
        .hash = relay_hash,
        .number = res_header.value().number,
        .storage_root = res_header.value().state_root,
    };
  }

  outcome::result<std::optional<
      std::pair<fragment::Constraints,
                std::vector<fragment::CandidatePendingAvailability>>>>
  ProspectiveParachains::fetchBackingState(const RelayHash &relay_parent,
                                           ParachainId para_id) {
    auto result =
        parachain_host_->staging_para_backing_state(relay_parent, para_id);
    if (result.has_error()) {
      SL_TRACE(logger,
               "Staging para backing state failed. (relay parent={}, "
               "para_id={}, error={})",
               relay_parent,
               para_id,
               result.error());
      return result.as_failure();
    }

    auto &s = result.value();
    if (!s) {
      return std::nullopt;
    }

    return std::make_pair(std::move(s->constraints),
                          std::move(s->pending_availability));
  }

  outcome::result<std::optional<runtime::PersistedValidationData>>
  ProspectiveParachains::answerProspectiveValidationDataRequest(
      const RelayHash &candidate_relay_parent,
      const ParentHeadData &parent_head_data,
      ParachainId para_id) {
    auto it = view.candidate_storage.find(para_id);
    if (it == view.candidate_storage.end()) {
      return std::nullopt;
    }

    const auto &storage = it->second;
    auto [head_data, parent_head_data_hash] = visit_in_place(
        parent_head_data,
        [&](const ParentHeadData_OnlyHash &parent_head_data_hash)
            -> std::pair<std::optional<HeadData>,
                         std::reference_wrapper<const Hash>> {
          return {utils::fromRefToOwn(
                      storage.headDataByHash(parent_head_data_hash)),
                  parent_head_data_hash};
        },
        [&](const ParentHeadData_WithData &v)
            -> std::pair<std::optional<HeadData>,
                         std::reference_wrapper<const Hash>> {
          const auto &[head_data, hash] = v;
          return {head_data, hash};
        });

    std::optional<fragment::RelayChainBlockInfo> relay_parent_info{};
    std::optional<size_t> max_pov_size{};

    for (const auto &[_, x] : view.active_leaves) {
      auto it = x.fragment_trees.find(para_id);
      if (it == x.fragment_trees.end()) {
        continue;
      }
      const fragment::FragmentTree &fragment_tree = it->second;

      if (head_data && relay_parent_info && max_pov_size) {
        break;
      }
      if (!relay_parent_info) {
        relay_parent_info = utils::fromRefToOwn(
            fragment_tree.scope.ancestorByHash(candidate_relay_parent));
      }
      if (!head_data) {
        const auto &required_parent =
            fragment_tree.scope.base_constraints.required_parent;
        if (hasher_->blake2b_256(required_parent)
            == parent_head_data_hash.get()) {
          head_data = required_parent;
        }
      }
      if (!max_pov_size) {
        if (fragment_tree.scope.ancestorByHash(candidate_relay_parent)) {
          max_pov_size = fragment_tree.scope.base_constraints.max_pov_size;
        }
      }
    }

    if (head_data && relay_parent_info && max_pov_size) {
      return runtime::PersistedValidationData{
          .parent_head = *head_data,
          .relay_parent_number = relay_parent_info->number,
          .relay_parent_storage_root = relay_parent_info->storage_root,
          .max_pov_size = (uint32_t)*max_pov_size,
      };
    }

    return std::nullopt;
  }

  outcome::result<std::unordered_set<ParachainId>>
  ProspectiveParachains::fetchUpcomingParas(
      const RelayHash &relay_parent,
      std::unordered_set<CandidateHash> &pending_availability) {
    OUTCOME_TRY(cores, parachain_host_->availability_cores(relay_parent));

    std::unordered_set<ParachainId> upcoming;
    for (const auto &core : cores) {
      visit_in_place(
          core,
          [&](const runtime::OccupiedCore &occupied) {
            pending_availability.insert(occupied.candidate_hash);
            if (occupied.next_up_on_available) {
              upcoming.insert(occupied.next_up_on_available->para_id);
            }
            if (occupied.next_up_on_time_out) {
              upcoming.insert(occupied.next_up_on_time_out->para_id);
            }
          },
          [&](const runtime::ScheduledCore &scheduled) {
            upcoming.insert(scheduled.para_id);
          },
          [](const auto &) {});
    }
    return upcoming;
  }

  outcome::result<std::vector<fragment::RelayChainBlockInfo>>
  ProspectiveParachains::fetchAncestry(const RelayHash &relay_hash,
                                       size_t ancestors) {
    std::vector<fragment::RelayChainBlockInfo> block_info;
    if (ancestors == 0) {
      return block_info;
    }

    OUTCOME_TRY(
        hashes,
        block_tree_->getDescendingChainToBlock(relay_hash, ancestors + 1));

    if (logger->level() >= soralog::Level::TRACE) {
      for (const auto &h : hashes) {
        SL_TRACE(logger,
                 "Ancestor hash. "
                 "(relay_hash={}, ancestor_hash={})",
                 relay_hash,
                 h);
      }
    }

    OUTCOME_TRY(required_session,
                parachain_host_->session_index_for_child(relay_hash));
    SL_TRACE(logger,
             "Get ancestors. "
             "(relay_hash={}, ancestors={}, hashes_len={})",
             relay_hash,
             ancestors,
             hashes.size());

    if (hashes.size() > 1) {
      block_info.reserve(hashes.size() - 1);
    }
    for (size_t i = 1; i < hashes.size(); ++i) {
      const auto &hash = hashes[i];
      OUTCOME_TRY(info, fetchBlockInfo(hash));
      if (!info) {
        SL_WARN(logger,
                "Failed to fetch info for hash returned from ancestry. "
                "(relay_hash={})",
                hash);
        break;
      }
      OUTCOME_TRY(session, parachain_host_->session_index_for_child(hash));
      if (session == required_session) {
        SL_TRACE(logger,
                 "Add block. "
                 "(relay_hash={}, hash={}, number={})",
                 relay_hash,
                 hash,
                 info->number);
        block_info.emplace_back(*info);
      } else {
        SL_TRACE(logger,
                 "Skipped block. "
                 "(relay_hash={}, hash={}, number={})",
                 relay_hash,
                 hash,
                 info->number);
        break;
      }
    }
    return block_info;
  }

  outcome::result<std::vector<ImportablePendingAvailability>>
  ProspectiveParachains::preprocessCandidatesPendingAvailability(
      const HeadData &required_parent,
      const std::vector<fragment::CandidatePendingAvailability>
          &pending_availability) {
    std::reference_wrapper<const HeadData> required_parent_copy =
        required_parent;
    std::vector<ImportablePendingAvailability> importable;
    const size_t expected_count = pending_availability.size();

    for (size_t i = 0; i < pending_availability.size(); i++) {
      const auto &pending = pending_availability[i];
      OUTCOME_TRY(relay_parent,
                  fetchBlockInfo(pending.descriptor.relay_parent));
      if (!relay_parent) {
        SL_DEBUG(logger,
                 "Had to stop processing pending candidates early due to "
                 "missing info. (candidate hash={}, parachain id={}, "
                 "index={}, expected count={})",
                 pending.candidate_hash,
                 pending.descriptor.para_id,
                 i,
                 expected_count);
        break;
      }

      const fragment::RelayChainBlockInfo &b = *relay_parent;
      importable.push_back(
          ImportablePendingAvailability{network::CommittedCandidateReceipt{
                                            pending.descriptor,
                                            pending.commitments,
                                        },
                                        runtime::PersistedValidationData{
                                            required_parent_copy.get(),
                                            b.number,
                                            b.storage_root,
                                            pending.max_pov_size,
                                        },
                                        fragment::PendingAvailability{
                                            pending.candidate_hash,
                                            b,
                                        }});
      required_parent_copy = pending.commitments.para_head;
    }
    return importable;
  }

  outcome::result<void> ProspectiveParachains::onActiveLeavesUpdate(
      const network::ExViewRef &update) {
    for (const auto &deactivated : update.lost) {
      SL_TRACE(
          logger, "Remove from active leaves. (relay_parent={})", deactivated);
      view.active_leaves.erase(deactivated);
    }

    /// TODO(iceseer): do https://github.com/qdrvm/kagome/issues/1888
    /// cache headers
    [[maybe_unused]] std::unordered_map<Hash, ProspectiveParachainsMode>
        temp_header_cache;
    if (update.new_head) {
      const auto &activated = update.new_head->get();
      const auto &hash = update.new_head->get().hash();
      const auto mode = prospectiveParachainsMode(hash);
      if (!mode) {
        SL_TRACE(logger,
                 "Skipping leaf activation since async backing is disabled. "
                 "(block_hash={})",
                 hash);
        return outcome::success();
      }
      std::unordered_set<Hash> pending_availability{};
      OUTCOME_TRY(scheduled_paras,
                  fetchUpcomingParas(hash, pending_availability));

      const fragment::RelayChainBlockInfo block_info{
          .hash = hash,
          .number = activated.number,
          .storage_root = activated.state_root,
      };

      OUTCOME_TRY(ancestry, fetchAncestry(hash, mode->allowed_ancestry_len));

      std::unordered_map<ParachainId, fragment::FragmentTree> fragment_trees;
      for (ParachainId para : scheduled_paras) {
        auto &candidate_storage = view.candidate_storage[para];
        OUTCOME_TRY(backing_state, fetchBackingState(hash, para));

        if (!backing_state) {
          SL_TRACE(logger,
                   "Failed to get inclusion backing state. (para={}, relay "
                   "parent={})",
                   para,
                   hash);
          continue;
        }
        const auto &[constraints, pe] = *backing_state;
        OUTCOME_TRY(pending_availability,
                    preprocessCandidatesPendingAvailability(
                        constraints.required_parent, pe));

        std::vector<fragment::PendingAvailability> compact_pending;
        compact_pending.reserve(pending_availability.size());

        for (const ImportablePendingAvailability &c : pending_availability) {
          const auto &candidate_hash = c.compact.candidate_hash;
          auto res = candidate_storage.addCandidate(
              candidate_hash,
              c.candidate,
              crypto::Hashed<const runtime::PersistedValidationData &,
                             32,
                             crypto::Blake2b_StreamHasher<32>>{
                  c.persisted_validation_data},
              hasher_);
          compact_pending.emplace_back(c.compact);

          if (res.has_value()
              || res.error()
                     == fragment::CandidateStorage::Error::
                         CANDIDATE_ALREADY_KNOWN) {
            candidate_storage.markBacked(candidate_hash);
          } else {
            SL_WARN(logger,
                    "Scraped invalid candidate pending availability. "
                    "(candidate_hash={}, para={}, error={})",
                    candidate_hash,
                    para,
                    res.error());
          }
        }

        OUTCOME_TRY(scope,
                    fragment::Scope::withAncestors(para,
                                                   block_info,
                                                   constraints,
                                                   compact_pending,
                                                   mode->max_candidate_depth,
                                                   ancestry));

        SL_TRACE(logger,
                 "Create fragment. "
                 "(relay_parent={}, para={}, min_relay_parent={})",
                 hash,
                 para,
                 scope.earliestRelayParent().number);
        fragment_trees.emplace(para,
                               fragment::FragmentTree::populate(
                                   hasher_, scope, candidate_storage));
      }

      SL_TRACE(logger, "Insert active leave. (relay parent={})", hash);
      view.active_leaves.emplace(
          hash, RelayBlockViewData{fragment_trees, pending_availability});
    }

    if (!update.lost.empty()) {
      prune_view_candidate_storage();
    }

    return outcome::success();
  }

  void ProspectiveParachains::prune_view_candidate_storage() {
    const auto &active_leaves = view.active_leaves;
    std::unordered_set<CandidateHash> live_candidates;
    std::unordered_set<ParachainId> live_paras;

    for (const auto &[_, sub_view] : active_leaves) {
      for (const auto &[para_id, fragment_tree] : sub_view.fragment_trees) {
        for (const auto &[ch, _] : fragment_tree.candidates) {
          live_candidates.insert(ch);
        }
        live_paras.insert(para_id);
      }

      live_candidates.insert(sub_view.pending_availability.begin(),
                             sub_view.pending_availability.end());
    }

    for (auto it = view.candidate_storage.begin();
         it != view.candidate_storage.end();) {
      auto &[para_id, storage] = *it;
      if (live_paras.find(para_id) != live_paras.end()) {
        storage.retain([&](const CandidateHash &h) {
          return live_candidates.find(h) != live_candidates.end();
        });
        ++it;
      } else {
        it = view.candidate_storage.erase(it);
      }
    }
  }

  std::vector<
      std::pair<HypotheticalCandidate, fragment::FragmentTreeMembership>>
  ProspectiveParachains::answerHypotheticalFrontierRequest(
      const std::span<const HypotheticalCandidate> &candidates,
      const std::optional<std::reference_wrapper<const Hash>>
          &fragment_tree_relay_parent,
      bool backed_in_path_only) {
    std::vector<
        std::pair<HypotheticalCandidate, fragment::FragmentTreeMembership>>
        response;
    response.reserve(candidates.size());
    std::transform(candidates.begin(),
                   candidates.end(),
                   std::back_inserter(response),
                   [](const HypotheticalCandidate &candidate)
                       -> std::pair<HypotheticalCandidate,
                                    fragment::FragmentTreeMembership> {
                     return {candidate, {}};
                   });

    const auto &required_active_leaf = fragment_tree_relay_parent;
    for (const auto &[active_leaf, leaf_view] : view.active_leaves) {
      if (required_active_leaf && required_active_leaf->get() != active_leaf) {
        continue;
      }

      for (auto &[c, membership] : response) {
        const ParachainId &para_id = candidatePara(c);
        auto it_fragment_tree = leaf_view.fragment_trees.find(para_id);
        if (it_fragment_tree == leaf_view.fragment_trees.end()) {
          continue;
        }

        auto it_candidate_storage = view.candidate_storage.find(para_id);
        if (it_candidate_storage == view.candidate_storage.end()) {
          continue;
        }

        const auto &fragment_tree = it_fragment_tree->second;
        const auto &candidate_storage = it_candidate_storage->second;
        const auto &candidate_hash = candidateHash(c);
        const auto &hypothetical = c;

        std::vector<size_t> depths =
            fragment_tree.hypotheticalDepths(candidate_hash,
                                             hypothetical,
                                             candidate_storage,
                                             backed_in_path_only);

        if (!depths.empty()) {
          membership.emplace_back(active_leaf, std::move(depths));
        }
      }
    }
    return response;
  }

  fragment::FragmentTreeMembership
  ProspectiveParachains::fragmentTreeMembership(
      const std::unordered_map<Hash, RelayBlockViewData> &active_leaves,
      ParachainId para,
      const CandidateHash &candidate) const {
    fragment::FragmentTreeMembership membership{};
    for (const auto &[relay_parent, view_data] : active_leaves) {
      if (auto it = view_data.fragment_trees.find(para);
          it != view_data.fragment_trees.end()) {
        const auto &tree = it->second;
        if (auto depths = tree.candidate(candidate)) {
          membership.emplace_back(relay_parent, *depths);
        }
      }
    }
    return membership;
  }

  void ProspectiveParachains::candidateSeconded(
      ParachainId para, const CandidateHash &candidate_hash) {
    auto it = view.candidate_storage.find(para);
    if (it == view.candidate_storage.end()) {
      SL_WARN(logger,
              "Received instruction to second unknown candidate. (para "
              "id={}, candidate hash={})",
              para,
              candidate_hash);
      return;
    }

    auto &storage = it->second;
    if (!storage.contains(candidate_hash)) {
      SL_WARN(logger,
              "Received instruction to second unknown candidate in storage. "
              "(para "
              "id={}, candidate hash={})",
              para,
              candidate_hash);
      return;
    }

    storage.markSeconded(candidate_hash);
  }

  void ProspectiveParachains::candidateBacked(
      ParachainId para, const CandidateHash &candidate_hash) {
    auto storage = view.candidate_storage.find(para);
    if (storage == view.candidate_storage.end()) {
      SL_WARN(logger,
              "Received instruction to back unknown candidate. (para_id={}, "
              "candidate_hash={})",
              para,
              candidate_hash);
      return;
    }
    if (!storage->second.contains(candidate_hash)) {
      SL_WARN(logger,
              "Received instruction to back unknown candidate. (para_id={}, "
              "candidate_hash={})",
              para,
              candidate_hash);
      return;
    }
    if (storage->second.isBacked(candidate_hash)) {
      SL_DEBUG(logger,
               "Received redundant instruction to mark candidate as backed. "
               "(para_id={}, candidate_hash={})",
               para,
               candidate_hash);
      return;
    }
    storage->second.markBacked(candidate_hash);
  }

  fragment::FragmentTreeMembership ProspectiveParachains::introduceCandidate(
      ParachainId para,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<const runtime::PersistedValidationData &,
                           32,
                           crypto::Blake2b_StreamHasher<32>> &pvd,
      const CandidateHash &candidate_hash) {
    auto it_storage = view.candidate_storage.find(para);
    if (it_storage == view.candidate_storage.end()) {
      SL_WARN(logger,
              "Received seconded candidate for inactive para. (parachain "
              "id={}, candidate hash={})",
              para,
              candidate_hash);
      return {};
    }

    auto &storage = it_storage->second;
    if (auto res =
            storage.addCandidate(candidate_hash, candidate, pvd, hasher_);
        res.has_error()) {
      if (res.error()
          == fragment::CandidateStorage::Error::CANDIDATE_ALREADY_KNOWN) {
        return fragmentTreeMembership(view.active_leaves, para, candidate_hash);
      }
      if (res.error()
          == fragment::CandidateStorage::Error::
              PERSISTED_VALIDATION_DATA_MISMATCH) {
        SL_WARN(logger,
                "Received seconded candidate had mismatching validation "
                "data. (parachain id={}, candidate hash={})",
                para,
                candidate_hash);
        return {};
      }
    }

    fragment::FragmentTreeMembership membership{};
    for (auto &[relay_parent, leaf_data] : view.active_leaves) {
      if (auto it = leaf_data.fragment_trees.find(para);
          it != leaf_data.fragment_trees.end()) {
        auto &tree = it->second;
        tree.addAndPopulate(candidate_hash, storage);
        if (auto depths = tree.candidate(candidate_hash)) {
          membership.emplace_back(relay_parent, *depths);
        }
      }
    }

    if (membership.empty()) {
      storage.removeCandidate(candidate_hash, hasher_);
    }

    return membership;
  }

}  // namespace kagome::parachain

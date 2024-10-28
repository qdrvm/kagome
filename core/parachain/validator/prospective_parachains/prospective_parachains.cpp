/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"
#include "utils/stringify.hpp"

#define COMPONENT ProspectiveParachains
#define COMPONENT_NAME STRINGIFY(COMPONENT)

template <>
struct fmt::formatter<
    std::vector<kagome::parachain::fragment::BlockInfoProspectiveParachains>> {
  constexpr auto parse(format_parse_context &ctx)
      -> format_parse_context::iterator {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(
      const std::vector<
          kagome::parachain::fragment::BlockInfoProspectiveParachains> &data,
      FormatContext &ctx) const -> decltype(ctx.out()) {
    auto out = fmt::format_to(ctx.out(), "[ ");
    for (const auto &i : data) {
      out = fmt::format_to(
          out,
          "BlockInfoProspectiveParachains {{ hash = {}, parent_hash = {}, "
          "number = {}, storage_root = {} }}, ",
          i.hash,
          i.parent_hash,
          i.number,
          i.storage_root);
    }
    return fmt::format_to(ctx.out(), "]");
  }
};

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
             "\n\t-> view.per_relay_parent={}"
             "\n\t-> view.active_leaves={}",
             view().per_relay_parent.size(),
             view().active_leaves.size());
  }

  std::shared_ptr<blockchain::BlockTree> ProspectiveParachains::getBlockTree() {
    BOOST_ASSERT(block_tree_);
    return block_tree_;
  }

  std::vector<std::pair<ParachainId, BlockNumber>>
  ProspectiveParachains::answerMinimumRelayParentsRequest(
      const RelayHash &relay_parent) {
    std::vector<std::pair<ParachainId, BlockNumber>> v;
    if (view().active_leaves.contains(relay_parent)) {
      if (auto leaf_data = utils::get(view().per_relay_parent, relay_parent)) {
        for (const auto &[para_id, fragment_chain] :
             leaf_data->get().fragment_chains) {
          v.emplace_back(
              para_id,
              fragment_chain.get_scope().earliest_relay_parent().number);
        }
      }
    }
    return v;
  }

  std::vector<std::pair<CandidateHash, Hash>>
  ProspectiveParachains::answerGetBackableCandidates(
      const RelayHash &relay_parent,
      ParachainId para,
      uint32_t count,
      const fragment::Ancestors &ancestors) {
    SL_TRACE(logger,
             "Search for backable candidates. (para_id={}, "
             "relay_parent={})",
             para,
             relay_parent);
    if (!view().active_leaves.contains(relay_parent)) {
      SL_TRACE(logger,
               "Requested backable candidate for inactive relay-parent. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
      return {};
    }

    auto data = utils::get(view().per_relay_parent, relay_parent);
    if (!data) {
      SL_TRACE(logger,
               "Requested backable candidate for inexistent relay-parent. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
      return {};
    }

    auto chain_it = utils::get(data->get().fragment_chains, para);
    if (!chain_it) {
      SL_TRACE(logger,
               "Requested backable candidate for inactive para. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
      return {};
    }

    auto &chain = chain_it->get();
    SL_TRACE(logger,
             "Candidate chain for para. "
             "(relay_parent={}, para_id={}, best chain size={})",
             relay_parent,
             para,
             chain.best_chain_len());

    auto backable_candidates = chain.find_backable_chain(ancestors, count);
    if (backable_candidates.empty()) {
      SL_TRACE(logger,
               "Could not find any backable candidate. "
               "(relay_parent={}, para_id={})",
               relay_parent,
               para);
    } else {
      SL_TRACE(logger,
               "Found backable candidates. "
               "(relay_parent={}, para_id={}, backable_candidates size={})",
               relay_parent,
               para,
               backable_candidates.size());
    }
    return backable_candidates;
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

  outcome::result<std::optional<fragment::BlockInfoProspectiveParachains>>
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

    return fragment::BlockInfoProspectiveParachains{
        .hash = relay_hash,
        .parent_hash = res_header.value().parent_hash,
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
    auto [head_data, parent_head_data_hash] = visit_in_place(
        parent_head_data,
        [&](const ParentHeadData_OnlyHash &parent_head_data_hash)
            -> std::pair<std::optional<HeadData>,
                         std::reference_wrapper<const Hash>> {
          return {std::nullopt, parent_head_data_hash};
        },
        [&](const ParentHeadData_WithData &v)
            -> std::pair<std::optional<HeadData>,
                         std::reference_wrapper<const Hash>> {
          const auto &[head_data, hash] = v;
          return {head_data, hash};
        });

    std::optional<fragment::RelayChainBlockInfo> relay_parent_info{};
    std::optional<size_t> max_pov_size{};

    for (const auto &x : view().active_leaves) {
      auto data = utils::get(view().per_relay_parent, x);
      if (!data) {
        continue;
      }

      auto fragment_chain_it = utils::get(data->get().fragment_chains, para_id);
      if (!fragment_chain_it) {
        continue;
      }

      auto &fragment_chain = fragment_chain_it->get();
      if (head_data && relay_parent_info && max_pov_size) {
        break;
      }

      if (!relay_parent_info) {
        relay_parent_info =
            fragment_chain.get_scope().ancestor(candidate_relay_parent);
      }

      if (!head_data) {
        head_data = fragment_chain.get_head_data_by_hash(parent_head_data_hash);
      }

      if (!max_pov_size) {
        if (fragment_chain.get_scope().ancestor(candidate_relay_parent)) {
          max_pov_size =
              fragment_chain.get_scope().get_base_constraints().max_pov_size;
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

  outcome::result<std::vector<fragment::BlockInfoProspectiveParachains>>
  ProspectiveParachains::fetchAncestry(const RelayHash &relay_hash,
                                       size_t ancestors) {
    std::vector<fragment::BlockInfoProspectiveParachains> block_info;
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

  outcome::result<
      std::vector<ProspectiveParachains::ImportablePendingAvailability>>
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

      const auto &b = *relay_parent;
      importable.push_back(ImportablePendingAvailability{
          .candidate =
              network::CommittedCandidateReceipt{
                  .descriptor = pending.descriptor,
                  .commitments = pending.commitments,
              },
          .persisted_validation_data =
              runtime::PersistedValidationData{
                  .parent_head = required_parent_copy.get(),
                  .relay_parent_number = b.number,
                  .relay_parent_storage_root = b.storage_root,
                  .max_pov_size = pending.max_pov_size,
              },
          .compact = fragment::PendingAvailability{
              .candidate_hash = pending.candidate_hash,
              .relay_parent = b.as_relay_chain_block_info(),
          }});
      required_parent_copy = pending.commitments.para_head;
    }
    return importable;
  }

  outcome::result<void> ProspectiveParachains::onActiveLeavesUpdate(
      const network::ExViewRef &update) {
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

      const fragment::BlockInfoProspectiveParachains block_info{
          .hash = hash,
          .parent_hash = activated.parent_hash,
          .number = activated.number,
          .storage_root = activated.state_root,
      };

      OUTCOME_TRY(ancestry, fetchAncestry(hash, mode->allowed_ancestry_len));

      std::optional<std::reference_wrapper<
          const std::unordered_map<ParachainId, fragment::FragmentChain>>>
          prev_fragment_chains;
      if (!ancestry.empty()) {
        const auto &prev_leaf = ancestry.front();
        prev_fragment_chains = view().get_fragment_chains(prev_leaf.hash);
      }

      std::unordered_map<ParachainId, fragment::FragmentChain> fragment_chains;
      for (ParachainId para : scheduled_paras) {
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

        fragment::CandidateStorage pending_availability_storage;
        for (const ImportablePendingAvailability &c : pending_availability) {
          const auto &candidate_hash = c.compact.candidate_hash;
          auto res =
              pending_availability_storage.add_pending_availability_candidate(
                  candidate_hash,
                  c.candidate,
                  c.persisted_validation_data,
                  hasher_);

          if (res.has_error()
              && res.error()
                     != fragment::CandidateStorage::Error::
                         CANDIDATE_ALREADY_KNOWN) {
            SL_WARN(logger,
                    "Scraped invalid candidate pending availability. "
                    "(candidate_hash={}, para={}, error={})",
                    candidate_hash,
                    para,
                    res.error());
            break;
          }
          compact_pending.emplace_back(c.compact);
        }

        std::vector<fragment::RelayChainBlockInfo> a;
        a.reserve(ancestry.size());
        for (const auto &ancestor : ancestry) {
          a.emplace_back(ancestor.as_relay_chain_block_info());
        }

        auto scope = fragment::Scope::with_ancestors(
            block_info.as_relay_chain_block_info(),
            constraints,
            compact_pending,
            mode->max_candidate_depth,
            a);
        if (scope.has_error()) {
          SL_WARN(logger,
                  "Relay chain ancestors have wrong order. "
                  "(para={}, max_candidate_depth={}, leaf={}, error={})",
                  para,
                  mode->max_candidate_depth,
                  hash,
                  scope.error());
          continue;
        }

        SL_TRACE(
            logger,
            "Creating fragment chain. "
            "(relay_parent={}, para={}, min_relay_parent={}, ancestors={})",
            hash,
            para,
            scope.value().earliest_relay_parent().number,
            ancestry);
        const auto number_of_pending_candidates =
            pending_availability_storage.len();
        auto chain = fragment::FragmentChain::init(
            hasher_, scope.value(), std::move(pending_availability_storage));

        if (chain.best_chain_len() < number_of_pending_candidates) {
          SL_WARN(
              logger,
              "Not all pending availability candidates could be introduced. "
              "(para={}, relay_parent={}, best_chain_len={}, "
              "number_of_pending_candidates={})",
              para,
              hash,
              chain.best_chain_len(),
              number_of_pending_candidates);
        }

        if (prev_fragment_chains) {
          if (auto prev_fragment_chain =
                  utils::get(prev_fragment_chains->get(), para)) {
            chain.populate_from_previous(prev_fragment_chain->get());
          }
        }

        SL_TRACE(
            logger,
            "Populated fragment chain. "
            "(relay_parent={}, para={}, best_chain_len={}, unconnected_len={})",
            hash,
            para,
            chain.best_chain_len(),
            chain.unconnected_len());

        fragment_chains.insert_or_assign(para, std::move(chain));
      }

      view().per_relay_parent.insert_or_assign(
          hash,
          RelayBlockViewData{.fragment_chains = std::move(fragment_chains)});
      view().active_leaves.insert(hash);
      view().implicit_view.activate_leaf_from_prospective_parachains(block_info,
                                                                     ancestry);
    }

    for (const auto &deactivated : update.lost) {
      view().active_leaves.erase(deactivated);
      view().implicit_view.deactivate_leaf(deactivated);
    }

    auto r = view().implicit_view.all_allowed_relay_parents();
    std::unordered_set<Hash> remaining{r.begin(), r.end()};

    for (auto it = view().per_relay_parent.begin();
         it != view().per_relay_parent.end();) {
      if (remaining.contains(it->first)) {
        ++it;
      } else {
        it = view().per_relay_parent.erase(it);
      }
    }

    return outcome::success();
  }

  ProspectiveParachains::View &ProspectiveParachains::view() {
    if (!view_) {
      view_.emplace(View{
          .per_relay_parent = {},
          .active_leaves = {},
          .implicit_view = ImplicitView(
              weak_from_this(), parachain_host_, block_tree_, std::nullopt),
      });
    }
    return *view_;
  }

  std::vector<
      std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
  ProspectiveParachains::answer_hypothetical_membership_request(
      const std::span<const HypotheticalCandidate> &candidates,
      const std::optional<std::reference_wrapper<const Hash>>
          &fragment_chain_relay_parent) {
    std::vector<
        std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
        response;
    response.reserve(candidates.size());

    std::ranges::transform(candidates,
                           std::back_inserter(response),
                           [](const HypotheticalCandidate &candidate)
                               -> std::pair<HypotheticalCandidate,
                                            fragment::HypotheticalMembership> {
                             return {candidate, {}};
                           });

    const auto &required_active_leaf = fragment_chain_relay_parent;
    for (const auto &active_leaf : view().active_leaves) {
      if (required_active_leaf && required_active_leaf->get() != active_leaf) {
        continue;
      }

      auto leaf_view = utils::get(view().per_relay_parent, active_leaf);
      if (!leaf_view) {
        continue;
      }

      for (auto &[candidate, membership] : response) {
        const ParachainId &para_id = candidatePara(candidate);

        auto fragment_chain =
            utils::get(leaf_view->get().fragment_chains, para_id);
        if (!fragment_chain) {
          continue;
        }

        const auto res = fragment_chain->get().can_add_candidate_as_potential(
            into_wrapper(candidate, hasher_));
        if (res.has_value()
            || res.error()
                   == fragment::FragmentChainError::CANDIDATE_ALREADY_KNOWN) {
          membership.emplace_back(active_leaf);
        } else {
          SL_TRACE(logger,
                   "Candidate is not a hypothetical member. "
                   "(para "
                   "id={}, leaf={}, error={})",
                   para_id,
                   active_leaf,
                   res.error());
        }
      }
    }
    return response;
  }

  void ProspectiveParachains::candidate_backed(
      ParachainId para, const CandidateHash &candidate_hash) {
    bool found_candidate = false;
    bool found_para = false;

    for (auto &[relay_parent, rp_data] : view().per_relay_parent) {
      auto chain = utils::get(rp_data.fragment_chains, para);
      if (!chain) {
        continue;
      }

      const auto is_active_leaf = view().active_leaves.contains(relay_parent);

      found_para = true;
      if (chain->get().is_candidate_backed(candidate_hash)) {
        SL_TRACE(
            logger,
            "Received redundant instruction to mark as backed an already "
            "backed candidate. (para={}, is_active_leaf={}, candidate_hash={})",
            para,
            is_active_leaf,
            candidate_hash);
        found_candidate = true;
      } else if (chain->get().contains_unconnected_candidate(candidate_hash)) {
        found_candidate = true;
        chain->get().candidate_backed(candidate_hash);

        SL_TRACE(logger,
                 "Candidate backed. Candidate chain for para. (para={}, "
                 "relay_parent={}, is_active_leaf={}, best_chain_len={})",
                 para,
                 relay_parent,
                 is_active_leaf,
                 chain->get().best_chain_len());

        SL_TRACE(logger,
                 "Potential candidate storage for para. (para={}, "
                 "relay_parent={}, is_active_leaf={}, unconnected_len={})",
                 para,
                 relay_parent,
                 is_active_leaf,
                 chain->get().unconnected_len());
      }
    }

    if (!found_para) {
      SL_WARN(logger,
              "Received instruction to back a candidate for unscheduled para. "
              "(para={}, candidate_hash={})",
              para,
              candidate_hash);
      return;
    }

    if (!found_candidate) {
      SL_TRACE(logger,
               "Received instruction to back unknown candidate. (para={}, "
               "candidate_hash={})",
               para,
               candidate_hash);
    }
  }

  bool ProspectiveParachains::introduce_seconded_candidate(
      ParachainId para,
      const network::CommittedCandidateReceipt &candidate,
      const crypto::Hashed<runtime::PersistedValidationData,
                           32,
                           crypto::Blake2b_StreamHasher<32>> &pvd,
      const CandidateHash &candidate_hash) {
    auto candidate_entry = fragment::CandidateEntry::create_seconded(
        candidate_hash, candidate, pvd, hasher_);
    if (candidate_entry.has_error()) {
      SL_WARN(logger,
              "Cannot add seconded candidate. (para={}, error={})",
              para,
              candidate_entry.error());
      return false;
    }

    bool added = false;
    bool para_scheduled = false;
    for (auto &[relay_parent, rp_data] : view().per_relay_parent) {
      auto chain = utils::get(rp_data.fragment_chains, para);
      if (!chain) {
        continue;
      }
      const auto is_active_leaf = view().active_leaves.contains(relay_parent);

      para_scheduled = true;
      if (auto res = chain->get().try_adding_seconded_candidate(
              candidate_entry.value());
          res.has_value()) {
        SL_TRACE(logger,
                 "Added seconded candidate. (para={}, relay_parent={}, "
                 "is_active_leaf={}, candidate_hash={})",
                 para,
                 relay_parent,
                 is_active_leaf,
                 candidate_hash);
        added = true;
      } else {
        if (res.error()
            == fragment::FragmentChainError::CANDIDATE_ALREADY_KNOWN) {
          SL_TRACE(
              logger,
              "Attempting to introduce an already known candidate. (para={}, "
              "relay_parent={}, is_active_leaf={}, candidate_hash={})",
              para,
              relay_parent,
              is_active_leaf,
              candidate_hash);
          added = true;
        } else {
          SL_TRACE(
              logger,
              "Cannot introduce seconded candidate. (para={}, relay_parent={}, "
              "is_active_leaf={}, candidate_hash={}, error={})",
              para,
              relay_parent,
              is_active_leaf,
              candidate_hash,
              res.error());
        }
      }
    }

    if (!para_scheduled) {
      SL_WARN(logger,
              "Received seconded candidate for inactive para. (para={}, "
              "candidate_hash={})",
              para,
              candidate_hash);
    }

    if (!added) {
      SL_TRACE(logger,
               "Newly-seconded candidate cannot be kept under any relay "
               "parent. (para={}, candidate_hash={})",
               para,
               candidate_hash);
    }
    return added;
  }

}  // namespace kagome::parachain

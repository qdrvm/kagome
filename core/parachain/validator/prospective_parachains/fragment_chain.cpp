/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include "utils/stringify.hpp"

namespace kagome::parachain::fragment {

  FragmentChain FragmentChain::init(
      std::shared_ptr<crypto::Hasher> hasher,
      const Scope &scope,
      CandidateStorage candidates_pending_availability) {
    FragmentChain fragment_chain{
        .scope = scope,
        .best_chain = {},
        .unconnected = {},
        .hasher_ = std::move(hasher),
    };

    fragment_chain.populate_chain(candidates_pending_availability);
    return fragment_chain;
  }

  size_t FragmentChain::best_chain_len() const {
    return best_chain.chain.size();
  }

  void FragmentChain::candidate_backed(
      const CandidateHash &newly_backed_candidate) {
    if (best_chain.candidates.contains(newly_backed_candidate)) {
      return;
    }

    auto it = unconnected.by_candidate_hash.find(newly_backed_candidate);
    if (it == unconnected.by_candidate_hash.end()) {
      return;
    }
    const auto parent_head_hash = it->second.parent_head_data_hash;

    unconnected.mark_backed(newly_backed_candidate);
    if (!revert_to(parent_head_hash)) {
      return;
    }

    auto prev_storage{std::move(unconnected)};
    populate_chain(prev_storage);

    trim_uneligible_forks(prev_storage, parent_head_hash);
    populate_unconnected_potential_candidates(std::move(prev_storage));
  }

  bool FragmentChain::is_candidate_backed(const CandidateHash &hash) const {
    return best_chain.candidates.contains(hash) || [&]() {
      auto it = unconnected.by_candidate_hash.find(hash);
      return it != unconnected.by_candidate_hash.end()
          && it->second.state == CandidateState::Backed;
    }();
  }

  Vec<CandidateHash> FragmentChain::best_chain_vec() const {
    Vec<CandidateHash> result;
    result.reserve(best_chain.chain.size());

    for (const auto &candidate : best_chain.chain) {
      result.emplace_back(candidate.candidate_hash);
    }
    return result;
  }

  bool FragmentChain::contains_unconnected_candidate(
      const CandidateHash &candidate) const {
    return unconnected.contains(candidate);
  }

  size_t FragmentChain::unconnected_len() const {
    return unconnected.len();
  }

  const Scope &FragmentChain::get_scope() const {
    return scope;
  }

  void FragmentChain::populate_from_previous(
      const FragmentChain &prev_fragment_chain) {
    auto prev_storage = prev_fragment_chain.unconnected;
    for (const auto &candidate : prev_fragment_chain.best_chain.chain) {
      if (!prev_fragment_chain.scope.get_pending_availability(
              candidate.candidate_hash)) {
        std::ignore =
            prev_storage.add_candidate_entry(candidate.into_candidate_entry());
      }
    }

    populate_chain(prev_storage);
    trim_uneligible_forks(prev_storage, std::nullopt);
    populate_unconnected_potential_candidates(std::move(prev_storage));
  }

  outcome::result<void> FragmentChain::check_not_contains_candidate(
      const CandidateHash &candidate_hash) const {
    if (best_chain.contains(candidate_hash)
        || unconnected.contains(candidate_hash)) {
      return FragmentChainError::CANDIDATE_ALREADY_KNOWN;
    }
    return outcome::success();
  }

  outcome::result<void> FragmentChain::check_cycles_or_invalid_tree(
      const Hash &output_head_hash) const {
    if (best_chain.by_parent_head.contains(output_head_hash)) {
      return FragmentChainError::CYCLE;
    }

    if (best_chain.by_output_head.contains(output_head_hash)) {
      return FragmentChainError::MULTIPLE_PATH;
    }

    return outcome::success();
  }

  Option<HeadData> FragmentChain::get_head_data_by_hash(
      const Hash &head_data_hash) const {
    const auto &required_parent = scope.get_base_constraints().required_parent;
    if (hasher_->blake2b_256(required_parent) == head_data_hash) {
      return required_parent;
    }

    const auto has_head_data_in_chain =
        best_chain.by_parent_head.contains(head_data_hash)
        || best_chain.by_output_head.contains(head_data_hash);
    if (has_head_data_in_chain) {
      for (const auto &candidate : best_chain.chain) {
        if (candidate.parent_head_data_hash == head_data_hash) {
          return candidate.fragment.get_candidate()
              .persisted_validation_data.parent_head;
        }

        if (candidate.output_head_data_hash == head_data_hash) {
          return candidate.fragment.get_candidate().commitments.para_head;
        }
      }
      return std::nullopt;
    }

    return utils::map(unconnected.head_data_by_hash(head_data_hash),
                      [](const auto &v) { return v.get(); });
  }

  void FragmentChain::populate_unconnected_potential_candidates(
      CandidateStorage old_storage) {
    for (auto &&[_, candidate] : old_storage.by_candidate_hash) {
      if (scope.get_pending_availability(candidate.candidate_hash)) {
        continue;
      }

      if (can_add_candidate_as_potential(candidate).has_value()) {
        std::ignore = unconnected.add_candidate_entry(std::move(candidate));
      }
    }
  }

  void FragmentChain::populate_chain(CandidateStorage &storage) {
    auto cumulative_modifications = [&]() {
      if (!best_chain.chain.empty()) {
        const auto &last_candidate = best_chain.chain.back();
        return last_candidate.cumulative_modifications;
      }
      return ConstraintModifications{
          .required_parent = std::nullopt,
          .hrmp_watermark = std::nullopt,
          .outbound_hrmp = {},
          .ump_messages_sent = 0,
          .ump_bytes_sent = 0,
          .dmp_messages_processed = 0,
          .code_upgrade_applied = false,
      };
    }();

    auto earliest_rp = earliest_relay_parent();
    if (!earliest_rp) {
      return;
    }

    for (;;) {
      if (best_chain.chain.size() > scope.max_depth) {
        break;
      }

      Constraints child_constraints;
      if (auto c = scope.base_constraints.apply_modifications(
              cumulative_modifications);
          c.has_value()) {
        child_constraints = std::move(c.value());
      } else {
        SL_TRACE(
            logger, "Failed to apply modifications. (error={})", c.error());
        break;
      }

      struct BestCandidate {
        Fragment fragment;
        CandidateHash candidate_hash;
        Hash output_head_data_hash;
        Hash parent_head_data_hash;
      };

      const auto required_head_hash =
          hasher_->blake2b_256(child_constraints.required_parent);
      Option<BestCandidate> best_candidate;

      storage.possible_backed_para_children(
          required_head_hash, [&](const auto &candidate) {
            auto pending =
                scope.get_pending_availability(candidate.candidate_hash);
            Option<RelayChainBlockInfo> relay_parent = utils::map(
                pending, [](const auto &v) { return v.get().relay_parent; });
            if (!relay_parent) {
              relay_parent = scope.ancestor(candidate.relay_parent);
            }
            if (!relay_parent) {
              return;
            }

            if (check_cycles_or_invalid_tree(candidate.output_head_data_hash)
                    .has_error()) {
              return;
            }

            BlockNumber min_relay_parent_number = earliest_rp->number;
            {
              auto mrp = utils::map(pending, [&](const auto &p) {
                if (best_chain.chain.empty()) {
                  return p.get().relay_parent.number;
                }
                return earliest_rp->number;
              });
              if (mrp) {
                min_relay_parent_number = *mrp;
              }
            }

            if (relay_parent->number < min_relay_parent_number) {
              return;
            }

            if (best_chain.contains(candidate.candidate_hash)) {
              return;
            }

            Fragment fragment;
            {
              auto constraints = child_constraints;
              if (pending) {
                constraints.min_relay_parent_number =
                    pending->get().relay_parent.number;
              }

              if (auto f = Fragment::create(
                      *relay_parent, constraints, candidate.candidate);
                  f.has_value()) {
                fragment = std::move(f.value());
              } else {
                SL_TRACE(logger,
                         "Failed to instantiate fragment. (error={}, "
                         "candidate_hash={})",
                         f.error(),
                         candidate.candidate_hash);
                return;
              }
            }

            if (!best_candidate
                || scope.get_pending_availability(candidate.candidate_hash)) {
              best_candidate = BestCandidate{
                  .fragment = std::move(fragment),
                  .candidate_hash = candidate.candidate_hash,
                  .output_head_data_hash = candidate.output_head_data_hash,
                  .parent_head_data_hash = candidate.parent_head_data_hash,
              };
            } else if (scope.get_pending_availability(
                           best_candidate->candidate_hash)) {
            } else {
              if (fork_selection_rule(candidate.candidate_hash,
                                      best_candidate->candidate_hash)) {
                best_candidate = BestCandidate{
                    .fragment = std::move(fragment),
                    .candidate_hash = candidate.candidate_hash,
                    .output_head_data_hash = candidate.output_head_data_hash,
                    .parent_head_data_hash = candidate.parent_head_data_hash,
                };
              }
            }
          });

      if (!best_candidate) {
        break;
      }

      storage.remove_candidate(best_candidate->candidate_hash, hasher_);
      cumulative_modifications.stack(
          best_candidate->fragment.constraint_modifications());
      earliest_rp = best_candidate->fragment.get_relay_parent();

      best_chain.push(FragmentNode{
          .fragment = std::move(best_candidate->fragment),
          .candidate_hash = best_candidate->candidate_hash,
          .cumulative_modifications = cumulative_modifications,
          .parent_head_data_hash = best_candidate->parent_head_data_hash,
          .output_head_data_hash = best_candidate->output_head_data_hash,
      });
    }
  }

  bool FragmentChain::fork_selection_rule(const CandidateHash &hash1,
                                          const CandidateHash &hash2) {
    return std::less<>()(hash1, hash2);
  }

  bool FragmentChain::revert_to(const Hash &parent_head_hash) {
    Option<Vec<FragmentNode>> removed_items;
    if (hasher_->blake2b_256(scope.base_constraints.required_parent)
        == parent_head_hash) {
      removed_items = best_chain.clear();
    }

    if (!removed_items
        && best_chain.by_output_head.contains(parent_head_hash)) {
      removed_items = best_chain.revert_to_parent_hash(parent_head_hash);
    }

    if (!removed_items) {
      return false;
    }
    for (const auto &node : *removed_items) {
      std::ignore =
          unconnected.add_candidate_entry(node.into_candidate_entry());
    }
    return true;
  }

  void FragmentChain::trim_uneligible_forks(CandidateStorage &storage,
                                            Option<Hash> starting_point) const {
    std::deque<std::pair<Hash, bool>> queue;
    if (starting_point) {
      queue.emplace_back(*starting_point, true);
    } else {
      if (best_chain.chain.empty()) {
        queue.emplace_back(
            hasher_->blake2b_256(scope.base_constraints.required_parent), true);
      } else {
        for (const auto &c : best_chain.chain) {
          queue.emplace_back(c.parent_head_data_hash, true);
        }
      }
    }

    auto pop = [&]() {
      Option<std::pair<Hash, bool>> result;
      if (!queue.empty()) {
        result = std::move(queue.front());
        queue.pop_front();
      }
      return result;
    };

    HashSet<Hash> visited;
    while (auto data = pop()) {
      const auto &[parent, parent_has_potential] = *data;
      visited.insert(parent);

      auto children = utils::get(storage.by_parent_head, parent);
      if (!children) {
        continue;
      }

      Vec<Hash> to_remove;
      for (const auto &child_hash : (*children)->second) {
        auto child = utils::get(storage.by_candidate_hash, child_hash);
        if (!child) {
          continue;
        }

        if (visited.contains((*child)->second.output_head_data_hash)) {
          continue;
        }

        if (parent_has_potential
            && check_potential((*child)->second).has_value()) {
          queue.emplace_back((*child)->second.output_head_data_hash, true);
        } else {
          to_remove.emplace_back(child_hash);
          queue.emplace_back((*child)->second.output_head_data_hash, false);
        }
      }

      for (const auto &hash : to_remove) {
        storage.remove_candidate(hash, hasher_);
      }
    }
  }

  RelayChainBlockInfo
  FragmentChain::earliest_relay_parent_pending_availability() const {
    for (const auto &candidate : std::ranges::reverse_view(best_chain.chain)) {
      auto item =
          utils::map(scope.get_pending_availability(candidate.candidate_hash),
                     [](const auto &v) { return v.get().relay_parent; });
      if (item) {
        return *item;
      }
    }
    return scope.earliest_relay_parent();
  }

  Option<RelayChainBlockInfo> FragmentChain::earliest_relay_parent() const {
    Option<RelayChainBlockInfo> result;
    if (!best_chain.chain.empty()) {
      const auto &last_candidate = best_chain.chain.back();
      result = scope.ancestor(last_candidate.relay_parent());
      if (!result) {
        result = utils::map(
            scope.get_pending_availability(last_candidate.candidate_hash),
            [](const auto &v) { return v.get().relay_parent; });
      }
    } else {
      result = scope.earliest_relay_parent();
    }
    return result;
  }

  size_t FragmentChain::find_ancestor_path(Ancestors ancestors) const {
    if (best_chain.chain.empty()) {
      return 0;
    }

    for (size_t index = 0; index < best_chain.chain.size(); ++index) {
      const auto &candidate = best_chain.chain[index];
      if (!ancestors.erase(candidate.candidate_hash)) {
        return index;
      }
    }
    return best_chain.chain.size();
  }

  Vec<std::pair<CandidateHash, Hash>> FragmentChain::find_backable_chain(
      Ancestors ancestors, uint32_t count) const {
    if (count == 0) {
      return {};
    }

    const auto base_pos = find_ancestor_path(std::move(ancestors));
    const auto actual_end_index =
        std::min(base_pos + size_t(count), best_chain.chain.size());

    Vec<std::pair<CandidateHash, Hash>> res;
    res.reserve(actual_end_index - base_pos);

    for (size_t ix = base_pos; ix < actual_end_index; ++ix) {
      const auto &elem = best_chain.chain[ix];
      if (!scope.get_pending_availability(elem.candidate_hash)) {
        res.emplace_back(elem.candidate_hash, elem.relay_parent());
      } else {
        break;
      }
    }
    return res;
  }

  outcome::result<void> FragmentChain::try_adding_seconded_candidate(
      const CandidateEntry &candidate) {
    if (candidate.state == CandidateState::Backed) {
      return FragmentChainError::INTRODUCE_BACKED_CANDIDATE;
    }

    OUTCOME_TRY(can_add_candidate_as_potential(candidate));
    return unconnected.add_candidate_entry(candidate);
  }

}  // namespace kagome::parachain::fragment

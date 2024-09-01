/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/backed_chain.hpp"
#include "parachain/validator/prospective_parachains/candidate_storage.hpp"
#include "parachain/validator/prospective_parachains/common.hpp"
#include "parachain/validator/prospective_parachains/scope.hpp"

namespace kagome::parachain::fragment {

  struct FragmentChain {
    enum Error {
      CANDIDATE_ALREADY_KNOWN = 1,
      INTRODUCE_BACKED_CANDIDATE,
      CYCLE,
      MULTIPLE_PATH,
    };

    // The current scope, which dictates the on-chain operating constraints that
    // all future candidates must adhere to.
    Scope scope;

    // The current best chain of backable candidates. It only contains
    // candidates which build on top of each other and which have reached the
    // backing quorum. In the presence of potential forks, this chain will pick
    // a fork according to the `fork_selection_rule`.
    BackedChain best_chain;

    // The potential candidate storage. Contains candidates which are not yet
    // part of the `chain` but may become in the future. These can form any tree
    // shape as well as contain any unconnected candidates for which we don't
    // know the parent.
    CandidateStorage unconnected;

    // Hasher
    std::shared_ptr<crypto::Hasher> hasher_;

    /// Create a new [`FragmentChain`] with the given scope and populate it with
    /// the candidates pending availability.
    static FragmentChain init(
        const Scope scope, CandidateStorage candidates_pending_availability) {
      FragmentChain fragment_chain{
          .scope = std::move(scope),
          .best_chain = {},
          .unconnected = {},
      };

      fragment_chain.populate_chain(candidates_pending_availability);
      return fragment_chain;
    }

    /// Populate the [`FragmentChain`] given the new candidates pending
    /// availability and the optional previous fragment chain (of the previous
    /// relay parent).
    void populate_from_previous(const FragmentChain &prev_fragment_chain) {
      auto prev_storage = prev_fragment_chain.unconnected;
      for (const auto &candidate : prev_fragment_chain.best_chain.chain) {
        if (!prev_fragment_chain.scope.get_pending_availability(
                candidate.candidate_hash)) {
          std::ignore = prev_storage.add_candidate_entry(
              candidate.into_candidate_entry());
        }
      }

      populate_chain(prev_storage);
      trim_uneligible_forks(prev_storage, std::nullopt);
      populate_unconnected_potential_candidates(std::move(prev_storage));
    }

    /// Get the scope of the [`FragmentChain`].
    const Scope &get_scope() const {
      return scope;
    }

    /// Returns the number of candidates in the best backable chain.
    size_t best_chain_len() const {
      return best_chain.chain.size();
    }

    /// Returns the number of candidates in unconnected potential storage.
    size_t unconnected_len() const {
      return unconnected.len();
    }

    /// Whether the candidate exists as part of the unconnected potential
    /// candidates.
    bool contains_unconnected_candidate(const CandidateHash &candidate) const {
      return unconnected.contains(candidate);
    }

    /// Return a vector of the chain's candidate hashes, in-order.
    Vec<CandidateHash> best_chain_vec() const {
      Vec<CandidateHash> result;
      result.reserve(best_chain.chain.size());

      for (const auto &candidate : best_chain.chain) {
        result.emplace_back(candidate.candidate_hash);
      }
      return result;
    }

    template <typename F>
    void get_unconnected(F &&callback /*void(const CandidateEntry &)*/) const {
      unconnected.candidates(std::forward<F>(callback));
    }

    /// Return whether this candidate is backed in this chain or the unconnected
    /// storage.
    bool is_candidate_backed(const CandidateHash &hash) const {
      return best_chain.candidates.contains(hash) || [&]() {
        auto it = unconnected.by_candidate_hash.find(hash);
        return it != unconnected.by_candidate_hash.end()
            && it->second.state == CandidateState::Backed;
      }();
    }

    /// Mark a candidate as backed. This can trigger a recreation of the best
    /// backable chain.
    void candidate_backed(const CandidateHash &newly_backed_candidate) {
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

    /// Checks if this candidate could be added in the future to this chain.
    /// This will return `Error::CandidateAlreadyKnown` if the candidate is
    /// already in the chain or the unconnected candidate storage.
    outcome::result<void> can_add_candidate_as_potential(
        const HypotheticalOrConcreteCandidate auto &candidate) const {
      OUTCOME_TRY(check_not_contains_candidate(candidate.get_candidate_hash()));
      return check_potential(candidate);
    }

    outcome::result<void> check_not_contains_candidate(
        const CandidateHash &candidate_hash) const;

    /// Try adding a seconded candidate, if the candidate has potential. It will
    /// never be added to the chain directly in the seconded state, it will only
    /// be part of the unconnected storage.
    outcome::result<void> try_adding_seconded_candidate(
        const CandidateEntry &candidate);

    /// Try getting the full head data associated with this hash.
    Option<HeadData> get_head_data_by_hash(const Hash &head_data_hash) const {
      const auto &required_parent =
          scope.get_base_constraints().required_parent;
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
          } else if (candidate.output_head_data_hash == head_data_hash) {
            return candidate.fragment.get_candidate().commitments.head_data;
          }
        }
        return std::nullopt;
      }

      return utils::map(unconnected.head_data_by_hash(head_data_hash),
                        [](const auto &v) { return v.get(); });
    }

    /// Select `count` candidates after the given `ancestors` which can be
    /// backed on chain next.
    ///
    /// The intention of the `ancestors` is to allow queries on the basis of
    /// one or more candidates which were previously pending availability
    /// becoming available or candidates timing out.
    Vec<std::pair<CandidateHash, Hash>> find_backable_chain(
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

    // Tries to orders the ancestors into a viable path from root to the last
    // one. Stops when the ancestors are all used or when a node in the chain is
    // not present in the ancestor set. Returns the index in the chain were the
    // search stopped.
    size_t find_ancestor_path(Ancestors ancestors) const {
      if (best_chain.chain.empty()) {
        return 0;
      }

      for (size_t index = 0; index < best_chain.chain.size(); ++index) {
        const auto &candidate = best_chain.chain[index];
        if (!ancestors.erase(candidate.candidate_hash)) {
          return index
        }
      }
      return best_chain.chain.size();
    }

    // Return the earliest relay parent a new candidate can have in order to be
    // added to the chain right now. This is the relay parent of the last
    // candidate in the chain. The value returned may not be valid if we want to
    // add a candidate pending availability, which may have a relay parent which
    // is out of scope. Special handling is needed in that case. `None` is
    // returned if the candidate's relay parent info cannot be found.
    Option<RelayChainBlockInfo> earliest_relay_parent() const {
      Option<RelayChainBlockInfo> result;
      if (!best_chain.chain.empty()) {
        const auto &last_candidate = best_chain.chain.back();
        result = scope.ancestor(last_candidate.relay_parent());
        if (!result) {
          result = utils::map(
              scope.get_pending_availability(last_candidate.candidate_hash),
              [](const auto &v) { return v.relay_parent; });
        }
      } else {
        result = scope.earliest_relay_parent();
      }
      return result;
    }

    // Return the earliest relay parent a potential candidate may have for it to
    // ever be added to the chain. This is the relay parent of the last
    // candidate pending availability or the earliest relay parent in scope.
    RelayChainBlockInfo earliest_relay_parent_pending_availability() const {
      for (auto it = best_chain.chain.rbegin(); it != best_chain.chain.rend();
           ++it) {
        const auto &candidate = *it;

        auto item =
            utils::map(scope.get_pending_availability(candidate.candidate_hash),
                       [](const auto &v) { return v.relay_parent; });
        if (item) {
          return *item;
        }
      }
      return scope.earliest_relay_parent();
    }

    // Populate the unconnected potential candidate storage starting from a
    // previous storage.
    void populate_unconnected_potential_candidates(
        CandidateStorage old_storage) {
      for (auto &&[_, candidate] : old_storage.by_candidate_hash) {
        if (scope.get_pending_availability(candidate.candidate_hash)) {
          continue;
        }

        if (can_add_candidate_as_potential(candidate).has_value()) {
          unconnected.add_candidate_entry(std::move(candidate));
        }
      }
    }

    // Check whether a candidate outputting this head data would introduce a
    // cycle or multiple paths to the same state. Trivial 0-length cycles are
    // checked in `CandidateEntry::new`.
    outcome::result<void> check_cycles_or_invalid_tree(
        const Hash &output_head_hash) const;
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, FragmentChain::Error)

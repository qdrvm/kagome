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
#include "parachain/validator/prospective_parachains/fragment_chain_errors.hpp"

namespace kagome::parachain::fragment {

  struct FragmentChain {
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

    // Logger
    log::Logger logger = log::createLogger("parachain", "fragment_chain");

    /// Create a new [`FragmentChain`] with the given scope and populate it with
    /// the candidates pending availability.
    static FragmentChain init(std::shared_ptr<crypto::Hasher> hasher,
                              const Scope scope,
                              CandidateStorage candidates_pending_availability);

    /// Populate the [`FragmentChain`] given the new candidates pending
    /// availability and the optional previous fragment chain (of the previous
    /// relay parent).
    void populate_from_previous(const FragmentChain &prev_fragment_chain);

    /// Get the scope of the [`FragmentChain`].
    const Scope &get_scope() const;

    /// Returns the number of candidates in the best backable chain.
    size_t best_chain_len() const;

    /// Returns the number of candidates in unconnected potential storage.
    size_t unconnected_len() const;

    /// Whether the candidate exists as part of the unconnected potential
    /// candidates.
    bool contains_unconnected_candidate(const CandidateHash &candidate) const;

    /// Return a vector of the chain's candidate hashes, in-order.
    Vec<CandidateHash> best_chain_vec() const;

    template <typename F>
    void get_unconnected(F &&callback /*void(const CandidateEntry &)*/) const {
      unconnected.candidates(std::forward<F>(callback));
    }

    /// Return whether this candidate is backed in this chain or the unconnected
    /// storage.
    bool is_candidate_backed(const CandidateHash &hash) const;

    /// Mark a candidate as backed. This can trigger a recreation of the best
    /// backable chain.
    void candidate_backed(const CandidateHash &newly_backed_candidate);

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
    Option<HeadData> get_head_data_by_hash(const Hash &head_data_hash) const;

    /// Select `count` candidates after the given `ancestors` which can be
    /// backed on chain next.
    ///
    /// The intention of the `ancestors` is to allow queries on the basis of
    /// one or more candidates which were previously pending availability
    /// becoming available or candidates timing out.
    Vec<std::pair<CandidateHash, Hash>> find_backable_chain(
        Ancestors ancestors, uint32_t count) const;

    // Tries to orders the ancestors into a viable path from root to the last
    // one. Stops when the ancestors are all used or when a node in the chain is
    // not present in the ancestor set. Returns the index in the chain were the
    // search stopped.
    size_t find_ancestor_path(Ancestors ancestors) const;

    // Return the earliest relay parent a new candidate can have in order to be
    // added to the chain right now. This is the relay parent of the last
    // candidate in the chain. The value returned may not be valid if we want to
    // add a candidate pending availability, which may have a relay parent which
    // is out of scope. Special handling is needed in that case. `None` is
    // returned if the candidate's relay parent info cannot be found.
    Option<RelayChainBlockInfo> earliest_relay_parent() const;

    // Return the earliest relay parent a potential candidate may have for it to
    // ever be added to the chain. This is the relay parent of the last
    // candidate pending availability or the earliest relay parent in scope.
    RelayChainBlockInfo earliest_relay_parent_pending_availability() const;

    // Populate the unconnected potential candidate storage starting from a
    // previous storage.
    void populate_unconnected_potential_candidates(
        CandidateStorage old_storage);

    // Check whether a candidate outputting this head data would introduce a
    // cycle or multiple paths to the same state. Trivial 0-length cycles are
    // checked in `CandidateEntry::new`.
    outcome::result<void> check_cycles_or_invalid_tree(
        const Hash &output_head_hash) const;

    // Checks the potential of a candidate to be added to the chain now or in
    // the future. It works both with concrete candidates for which we have the
    // full PVD and committed receipt, but also does some more basic checks for
    // incomplete candidates (before even fetching them).
  outcome::result<void> check_potential(
      const HypotheticalOrConcreteCandidate auto &candidate) const {
    const auto parent_head_hash = candidate.get_parent_head_data_hash();

    if (auto output_head_hash = candidate.get_output_head_data_hash()) {
      if (parent_head_hash == *output_head_hash) {
        return FragmentChainError::ZERO_LENGTH_CYCLE;
      }
    }

    auto relay_parent = scope.ancestor(candidate.get_relay_parent());
    if (!relay_parent) {
      return FragmentChainError::RELAY_PARENT_NOT_IN_SCOPE;
    }

    const auto earliest_rp_of_pending_availability =
        earliest_relay_parent_pending_availability();
    if (relay_parent->number < earliest_rp_of_pending_availability.number) {
      return FragmentChainError::RELAY_PARENT_PRECEDES_CANDIDATE_PENDING_AVAILABILITY;
    }

    if (auto other_candidate =
            utils::get(best_chain.by_parent_head, parent_head_hash)) {
      if (scope.get_pending_availability((*other_candidate)->second)) {
        return FragmentChainError::FORK_WITH_CANDIDATE_PENDING_AVAILABILITY;
      }

      if (fork_selection_rule((*other_candidate)->second,
                              candidate.get_candidate_hash())) {
        return FragmentChainError::FORK_CHOICE_RULE;
      }
    }

    std::pair<Constraints, Option<BlockNumber>> constraints_and_number;
    if (auto parent_candidate_ =
            utils::get(best_chain.by_output_head, parent_head_hash)) {
      auto parent_candidate = std::find_if(
          best_chain.chain.begin(), best_chain.chain.end(), [&](const auto &c) {
            return c.candidate_hash == (*parent_candidate_)->second;
          });
      if (parent_candidate == best_chain.chain.end()) {
        return FragmentChainError::PARENT_CANDIDATE_NOT_FOUND;
      }

      auto constraints = scope.base_constraints.apply_modifications(
          parent_candidate->cumulative_modifications);
      if (constraints.has_error()) {
        return FragmentChainError::COMPUTE_CONSTRAINTS;
      }
      constraints_and_number = std::make_pair(
          std::move(constraints.value()),
          utils::map(scope.ancestor(parent_candidate->relay_parent()),
                     [](const auto &rp) { return rp.number; }));
    } else if (hasher_->blake2b_256(scope.base_constraints.required_parent)
               == parent_head_hash) {
      constraints_and_number =
          std::make_pair(scope.base_constraints, std::nullopt);
    } else {
      return outcome::success();
    }
    const auto &[constraints, maybe_min_relay_parent_number] =
        constraints_and_number;

    if (auto output_head_hash = candidate.get_output_head_data_hash()) {
      OUTCOME_TRY(check_cycles_or_invalid_tree(*output_head_hash));
    }

    if (candidate.get_commitments() && candidate.get_persisted_validation_data()
        && candidate.get_validation_code_hash()) {
      if (Fragment::check_against_constraints(
              *relay_parent,
              constraints,
              candidate.get_commitments()->get(),
              candidate.get_validation_code_hash()->get(),
              candidate.get_persisted_validation_data()->get())
              .has_error()) {
        return FragmentChainError::CHECK_AGAINST_CONSTRAINTS;
      }
    }

    if (relay_parent->number < constraints.min_relay_parent_number) {
      return FragmentChainError::RELAY_PARENT_MOVED_BACKWARDS;
    }

    if (maybe_min_relay_parent_number) {
      if (relay_parent->number < *maybe_min_relay_parent_number) {
        return FragmentChainError::RELAY_PARENT_MOVED_BACKWARDS;
      }
    }

    return outcome::success();
  }

    // Once the backable chain was populated, trim the forks generated by
    // candidates which are not present in the best chain. Fan this out into a
    // full breadth-first search. If `starting_point` is `Some()`, start the
    // search from the candidates having this parent head hash.
    void trim_uneligible_forks(CandidateStorage &storage,
                               Option<Hash> starting_point) const;

    // Revert the best backable chain so that the last candidate will be one
    // outputting the given `parent_head_hash`. If the `parent_head_hash` is
    // exactly the required parent of the base constraints (builds on the latest
    // included candidate), revert the entire chain. Return false if we couldn't
    // find the parent head hash.
    bool revert_to(const Hash &parent_head_hash);

    /// The rule for selecting between two backed candidate forks, when adding
    /// to the chain. All validators should adhere to this rule, in order to not
    /// lose out on rewards in case of forking parachains.
    static bool fork_selection_rule(const CandidateHash &hash1,
                                    const CandidateHash &hash2);

    // Populate the fragment chain with candidates from the supplied
    // `CandidateStorage`. Can be called by the constructor or when backing a
    // new candidate. When this is called, it may cause the previous chain to be
    // completely erased or it may add more than one candidate.
    void populate_chain(CandidateStorage &storage);
  };

}  // namespace kagome::parachain::fragment

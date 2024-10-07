/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/common.hpp"

namespace kagome::parachain::fragment {

  /// A candidate existing on-chain but pending availability, for special
  /// treatment in the [`Scope`].
  struct PendingAvailability {
    /// The candidate hash.
    CandidateHash candidate_hash;
    /// The block info of the relay parent.
    RelayChainBlockInfo relay_parent;
  };

  /// The scope of a [`FragmentChain`].
  struct Scope {
    enum class Error : uint8_t {
      UNEXPECTED_ANCESTOR,
    };

    /// The relay parent we're currently building on top of.
    RelayChainBlockInfo relay_parent;

    /// The other relay parents candidates are allowed to build upon, mapped by
    /// the block number.
    Map<BlockNumber, RelayChainBlockInfo> ancestors;

    /// The other relay parents candidates are allowed to build upon, mapped by
    /// the block hash.
    HashMap<Hash, RelayChainBlockInfo> ancestors_by_hash;

    /// The candidates pending availability at this block.
    Vec<PendingAvailability> pending_availability;

    /// The base constraints derived from the latest included candidate.
    Constraints base_constraints;

    /// Equal to `max_candidate_depth`.
    size_t max_depth;

    /// Define a new [`Scope`].
    ///
    /// Ancestors should be in reverse order, starting with the parent
    /// of the `relay_parent`, and proceeding backwards in block number
    /// increments of 1. Ancestors not following these conditions will be
    /// rejected.
    ///
    /// This function will only consume ancestors up to the
    /// `min_relay_parent_number` of the `base_constraints`.
    ///
    /// Only ancestors whose children have the same session as the
    /// relay-parent's children should be provided.
    ///
    /// It is allowed to provide zero ancestors.
    static outcome::result<Scope> with_ancestors(
        const RelayChainBlockInfo &relay_parent,
        const Constraints &base_constraints,
        const Vec<PendingAvailability> &pending_availability,
        size_t max_depth,
        const Vec<RelayChainBlockInfo> &ancestors);

    /// Get the base constraints of the scope
    const Constraints &get_base_constraints() const;

    /// Get the earliest relay-parent allowed in the scope of the fragment
    /// chain.
    const RelayChainBlockInfo earliest_relay_parent() const;

    /// Whether the candidate in question is one pending availability in this
    /// scope.
    Option<std::reference_wrapper<const PendingAvailability>>
    get_pending_availability(const CandidateHash &candidate_hash) const;

    /// Get the relay ancestor of the fragment chain by hash.
    Option<RelayChainBlockInfo> ancestor(const Hash &hash) const;
  };

}  // namespace kagome::parachain::fragment

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain::fragment, Scope::Error);

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/scope.hpp"
#include "utils/stringify.hpp"

#define COMPONENT Scope
#define COMPONENT_NAME STRINGIFY(COMPONENT)

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment, Scope::Error, e) {
  using E = kagome::parachain::fragment::Scope::Error;
  switch (e) {
    case E::UNEXPECTED_ANCESTOR:
      return COMPONENT_NAME ": Unexpected ancestor";
  }
  return COMPONENT_NAME ": Unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<Scope> Scope::with_ancestors(
      const fragment::RelayChainBlockInfo &relay_parent,
      const Constraints &base_constraints,
      const Vec<PendingAvailability> &pending_availability,
      size_t max_depth,
      const Vec<fragment::RelayChainBlockInfo> &ancestors) {
    Map<BlockNumber, fragment::RelayChainBlockInfo> ancestors_map;
    HashMap<Hash, fragment::RelayChainBlockInfo> ancestors_by_hash;

    auto prev = relay_parent.number;
    for (const auto &ancestor : ancestors) {
      if (prev == 0) {
        return Scope::Error::UNEXPECTED_ANCESTOR;
      }
      if (ancestor.number != prev - 1) {
        return Scope::Error::UNEXPECTED_ANCESTOR;
      }
      if (prev == base_constraints.min_relay_parent_number) {
        break;
      }

      prev = ancestor.number;
      ancestors_by_hash[ancestor.hash] = ancestor;
      ancestors_map[ancestor.number] = ancestor;
    }

    return Scope{
        .relay_parent = relay_parent,
        .ancestors = ancestors_map,
        .ancestors_by_hash = ancestors_by_hash,
        .pending_availability = pending_availability,
        .base_constraints = base_constraints,
        .max_depth = max_depth,
    };
  }

  const Constraints &Scope::get_base_constraints() const {
    return base_constraints;
  }

  const RelayChainBlockInfo Scope::earliest_relay_parent() const {
    if (!ancestors.empty()) {
      return ancestors.begin()->second;
    }
    return relay_parent;
  }

  Option<std::reference_wrapper<const PendingAvailability>>
  Scope::get_pending_availability(const CandidateHash &candidate_hash) const {
    auto it = std::ranges::find_if(pending_availability.begin(),
                                   pending_availability.end(),
                                   [&](const PendingAvailability &c) {
                                     return c.candidate_hash == candidate_hash;
                                   });
    if (it != pending_availability.end()) {
      return {{*it}};
    }
    return std::nullopt;
  }

  Option<RelayChainBlockInfo> Scope::ancestor(const Hash &hash) const {
    if (hash == relay_parent.hash) {
      return relay_parent;
    }
    if (auto it = ancestors_by_hash.find(hash); it != ancestors_by_hash.end()) {
      return it->second;
    }
    return std::nullopt;
  }

}  // namespace kagome::parachain::fragment

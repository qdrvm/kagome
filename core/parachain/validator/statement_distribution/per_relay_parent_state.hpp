
/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>
#include <memory>
#include <unordered_map>

#include "parachain/backing/cluster.hpp"
#include "parachain/backing/grid_tracker.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/impl/statements_store.hpp"
#include "utils/safe_object.hpp"

namespace kagome::parachain::statement_distribution {

  /// polkadot/node/network/statement-distribution/src/v2/mod.rs
  struct ActiveValidatorState {
    // The index of the validator.
    ValidatorIndex index;
    // our validator group
    GroupIndex group;
    // the assignment of our validator group, if any.
    std::optional<ParachainId> assignment;
    // the 'direct-in-group' communication at this relay-parent.
    ClusterTracker cluster_tracker;
  };

  struct LocalValidatorState {
    // the grid-level communication at this relay-parent.
    grid::GridTracker grid_tracker;
    // additional fields in case local node is an active validator.
    std::optional<ActiveValidatorState> active;
  };

  struct PerRelayParentState {
    std::optional<LocalValidatorState> local_validator;
    StatementStore statement_store;
    size_t seconding_limit;
    SessionIndex session;
    std::unordered_map<ParachainId, std::vector<GroupIndex>> groups_per_para;
    std::unordered_set<ValidatorIndex> disabled_validators;

    std::optional<std::reference_wrapper<const ActiveValidatorState>>
    active_validator_state() const {
      if (local_validator && local_validator->active) {
        return local_validator->active.value();
      }
      return std::nullopt;
    }

    bool is_disabled(ValidatorIndex validator_index) const {
      return disabled_validators.contains(validator_index);
    }

    scale::BitVec disabled_bitmask(
        const std::span<const ValidatorIndex> &group) const {
      scale::BitVec v;
      v.bits.resize(group.size());
      for (size_t ix = 0; ix < group.size(); ++ix) {
        v.bits[ix] = is_disabled(group[ix]);
      }
      return v;
    }
  };

}  // namespace kagome::parachain::statement_distribution

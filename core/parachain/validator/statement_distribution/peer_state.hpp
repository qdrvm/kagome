/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>

#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace kagome::parachain {
  class ParachainProcessor;
}

namespace kagome::parachain::statement_distribution {

  struct PeerState {
    network::View view;
    std::unordered_set<common::Hash256> implicit_view;

    /// Update the view, returning a vector of implicit relay-parents which
    /// weren't previously part of the view.
    std::vector<common::Hash256> update_view(
        const network::View &new_view,
        const parachain::ImplicitView &local_implicit) {
      std::unordered_set<common::Hash256> next_implicit;
      for (const auto &x : new_view.heads_) {
        auto t =
            local_implicit.known_allowed_relay_parents_under(x, std::nullopt);
        if (t) {
          next_implicit.insert(t->begin(), t->end());
        }
      }

      std::vector<common::Hash256> fresh_implicit;
      for (const auto &x : next_implicit) {
        if (implicit_view.find(x) == implicit_view.end()) {
          fresh_implicit.emplace_back(x);
        }
      }

      view = new_view;
      implicit_view = next_implicit;
      return fresh_implicit;
    }

    /// Whether we know that a peer knows a relay-parent. The peer knows the
    /// relay-parent if it is either implicit or explicit in their view.
    /// However, if it is implicit via an active-leaf we don't recognize, we
    /// will not accurately be able to recognize them as 'knowing' the
    /// relay-parent.
    bool knows_relay_parent(const common::Hash256 &relay_parent) {
      return implicit_view.contains(relay_parent)
          || view.contains(relay_parent);
    }

    /// Attempt to reconcile the view with new information about the implicit
    /// relay parents under an active leaf.
    std::vector<common::Hash256> reconcile_active_leaf(
        const common::Hash256 &leaf_hash,
        std::span<const common::Hash256> implicit) {
      if (!view.contains(leaf_hash)) {
        return {};
      }

      std::vector<common::Hash256> v;
      v.reserve(implicit.size());
      for (const auto &i : implicit) {
        auto [_, inserted] = implicit_view.insert(i);
        if (inserted) {
          v.emplace_back(i);
        }
      }
      return v;
    }
  };

}  // namespace kagome::parachain::statement_distribution

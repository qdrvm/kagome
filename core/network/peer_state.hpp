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

#include "consensus/grandpa/common.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "outcome/outcome.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "primitives/common.hpp"
#include "utils/lru.hpp"

namespace kagome::network {

  constexpr size_t kPeerStateMaxKnownBlocks = 1024;
  constexpr size_t kPeerStateMaxKnownGrandpaMessages = 8192;
  using RoundNumber = consensus::grandpa::RoundNumber;
  using VoterSetId = consensus::grandpa::VoterSetId;
  using BlockInfo = primitives::BlockInfo;

  struct CollatingPeerState {
    network::ParachainId para_id;
    network::CollatorPublicKey collator_id;
    std::unordered_map<Hash, std::unordered_set<CandidateHash>> advertisements;
    std::chrono::system_clock::time_point last_active;
  };

  using OurView = network::View;

  struct PeerStateCompact {
    std::optional<RoundNumber> round_number;
    std::optional<VoterSetId> set_id;
    BlockNumber last_finalized;
  };

  struct PeerState {
    clock::SteadyClock::TimePoint time;
    Roles roles = 0;
    BlockInfo best_block = {0, {}};
    std::optional<RoundNumber> round_number = std::nullopt;
    std::optional<VoterSetId> set_id = std::nullopt;
    BlockNumber last_finalized = 0;
    LruSet<primitives::BlockHash> known_blocks{kPeerStateMaxKnownBlocks};
    LruSet<common::Hash256> known_grandpa_messages{
        kPeerStateMaxKnownGrandpaMessages,
    };

    /// @brief parachain peer state
    std::optional<CollatingPeerState> collator_state = std::nullopt;
    View view;
    std::unordered_set<common::Hash256> implicit_view;
    std::optional<CollationVersion> collation_version;
    std::optional<ReqChunkVersion> req_chunk_version;

    /// Update the view, returning a vector of implicit relay-parents which
    /// weren't previously part of the view.
    std::vector<common::Hash256> update_view(
        const View &new_view, const parachain::ImplicitView &local_implicit) {
      std::unordered_set<common::Hash256> next_implicit;
      for (const auto &x : new_view.heads_) {
        auto t = local_implicit.knownAllowedRelayParentsUnder(x, std::nullopt);
        next_implicit.insert(t.begin(), t.end());
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

    /// Whether the peer has advertised the given collation.
    bool hasAdvertised(
        const RelayHash &relay_parent,
        const std::optional<CandidateHash> &maybe_candidate_hash) const {
      if (!collator_state) {
        return false;
      }

      const auto &collating_state = *collator_state;
      if (maybe_candidate_hash) {
        if (auto it = collating_state.advertisements.find(relay_parent);
            it != collating_state.advertisements.end()) {
          return it->second.contains(*maybe_candidate_hash);
        }
        return false;
      }
      return collating_state.advertisements.contains(relay_parent);
    }

    PeerStateCompact compact() const {
      return PeerStateCompact{
          .round_number = round_number,
          .set_id = set_id,
          .last_finalized = last_finalized,
      };
    }
  };

  inline std::optional<PeerStateCompact> compactFromRefToOwn(
      const std::optional<std::reference_wrapper<PeerState>> &opt_ref) {
    if (opt_ref) {
      return opt_ref->get().compact();
    }
    return std::nullopt;
  }

}  // namespace kagome::network

template <>
struct std::hash<kagome::network::FetchedCollation> {
  size_t operator()(
      const kagome::network::FetchedCollation &value) const {
    using CollatorId = kagome::parachain::CollatorId;
    using CandidateHash = kagome::parachain::CandidateHash;
    using RelayHash = kagome::parachain::RelayHash;
    using ParachainId = kagome::parachain::ParachainId;

    size_t result = std::hash<RelayHash>()(value.relay_parent);
    boost::hash_combine(result, std::hash<ParachainId>()(value.para_id));
    boost::hash_combine(result,
                        std::hash<CandidateHash>()(value.candidate_hash));
    boost::hash_combine(result, std::hash<CollatorId>()(value.collator_id));

    return result;
  }
};

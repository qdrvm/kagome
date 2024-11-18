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
#include "network/types/roles.hpp"
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
    std::optional<CollationVersion> collation_version;
    std::optional<ReqChunkVersion> req_chunk_version;

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
  size_t operator()(const kagome::network::FetchedCollation &value) const {
    using CandidateHash = kagome::parachain::CandidateHash;
    using RelayHash = kagome::parachain::RelayHash;
    using ParachainId = kagome::parachain::ParachainId;

    size_t result = std::hash<RelayHash>()(value.relay_parent);
    boost::hash_combine(result, std::hash<ParachainId>()(value.para_id));
    boost::hash_combine(result,
                        std::hash<CandidateHash>()(value.candidate_hash));

    return result;
  }
};

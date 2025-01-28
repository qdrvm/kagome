/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/structs.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  using consensus::grandpa::CompactCommit;
  using consensus::grandpa::GrandpaJustification;
  using consensus::grandpa::RoundNumber;
  using consensus::grandpa::SignedMessage;
  using consensus::grandpa::SignedPrecommit;
  using consensus::grandpa::SignedPrevote;
  using consensus::grandpa::VoteMessage;
  using consensus::grandpa::VoterSetId;
  using primitives::BlockHash;
  using primitives::BlockInfo;
  using primitives::BlockNumber;

  using GrandpaVote = VoteMessage;

  // Network level commit message with topic information.
  // @See
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.7/client/finality-grandpa/src/communication/gossip.rs#L350
  struct FullCommitMessage {
    // The round this message is from.
    RoundNumber round{0};
    // The voter set ID this message is from.
    VoterSetId set_id;
    // The compact commit message.
    CompactCommit message;
  };

  struct GrandpaNeighborMessage {
    uint8_t version = 1;
    RoundNumber round_number;
    VoterSetId voter_set_id;
    BlockNumber last_finalized;
    bool operator==(const GrandpaNeighborMessage &other) const = default;
  };

  struct CatchUpRequest {
    RoundNumber round_number;
    VoterSetId voter_set_id;

    using Fingerprint = size_t;

    inline Fingerprint fingerprint() const {
      auto result = std::hash<RoundNumber>()(round_number);

      boost::hash_combine(result, std::hash<VoterSetId>()(voter_set_id));
      return result;
    }
  };

  struct CatchUpResponse {
    VoterSetId voter_set_id{};
    RoundNumber round_number{};
    std::vector<SignedPrevote> prevote_justification;
    std::vector<SignedPrecommit> precommit_justification;
    BlockInfo best_final_candidate;
  };

  // @See
  // https://github.com/paritytech/substrate/blob/polkadot-v0.9.7/client/finality-grandpa/src/communication/gossip.rs#L318
  using GrandpaMessage =
      /// Note: order of types in variant matters
      boost::variant<GrandpaVote,             // 0
                     FullCommitMessage,       // 1
                     GrandpaNeighborMessage,  // 2
                     CatchUpRequest,          // 3
                     CatchUpResponse>;        // 4

}  // namespace kagome::network

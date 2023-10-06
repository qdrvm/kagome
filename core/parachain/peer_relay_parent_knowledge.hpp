/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PEER_RP_KNOWLEDGE_HPP
#define KAGOME_PARACHAIN_PEER_RP_KNOWLEDGE_HPP

#include <unordered_map>
#include <unordered_set>

#include "network/types/collator_messages.hpp"
#include "parachain/candidate_view.hpp"

namespace kagome::parachain {

  /// knowledge that a peer has about goings-on in a relay parent.
  struct PeerRelayParentKnowledge {
    using CandidateHash = network::CandidateHash;
    struct PeerStatement {
      SCALE_TIE(2);
      network::CompactStatement compact_statement;
      network::ValidatorIndex validator_index;
    };

    std::unordered_set<CandidateHash>
        sent_candidates;  /// candidates that the peer is aware of because we
                          /// sent statements to it. This indicates that we can
                          /// send other statements pertaining to that
                          /// candidate.
    std::unordered_set<CandidateHash>
        received_candidates;  /// candidates that peer is aware of, because we
                              /// received statements from it.
    std::unordered_set<PeerStatement>
        sent_statements;  /// fingerprints of all statements a peer should be
                          /// aware of: those that were sent to the peer by us.
    std::unordered_set<PeerStatement>
        received_statements;  /// fingerprints of all statements a peer should
                              /// be aware of: those that were sent to us by the
                              /// peer.
    std::unordered_map<network::ValidatorIndex, VcPerPeerTracker>
        seconded_counts;  /// How many candidates this peer is aware of for each
                          /// given validator index.
    std::unordered_map<network::CandidateHash, uint64_t>
        received_message_count;  /// How many statements we've received for each
                                 /// candidate that we're aware of.
    uint64_t large_statement_count;  /// How many large statements this peer
                                     /// already sent us.
    uint64_t
        unexpected_count;  /// We have seen a message that that is unexpected
                           /// from this peer, so note this fact and stop
                           /// subsequent logging and peer reputation flood.
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PEER_RP_KNOWLEDGE_HPP

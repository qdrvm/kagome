/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPAMESSAGE
#define KAGOME_NETWORK_GRANDPAMESSAGE

#include <boost/variant.hpp>

#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/common.hpp"

namespace kagome::network {

  using consensus::grandpa::BlockInfo;
  using consensus::grandpa::GrandpaJustification;
  using consensus::grandpa::GrandpaJustification;
  using consensus::grandpa::RoundNumber;
  using consensus::grandpa::SignedMessage;
  using consensus::grandpa::SignedPrecommit;
  using consensus::grandpa::SignedPrevote;
  using consensus::grandpa::VoteMessage;
  using consensus::grandpa::CompactCommit;
  using consensus::grandpa::VoterSetId;
  using primitives::BlockNumber;

  struct GrandpaVote : public VoteMessage {
    using VoteMessage::VoteMessage;
    explicit GrandpaVote(VoteMessage &&vm) noexcept
        : VoteMessage(std::move(vm)){};
  };

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

  template <class Stream,
      typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const FullCommitMessage &f) {
    return s << f.round << f.set_id << f.message;
  }

  template <class Stream,
      typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, FullCommitMessage &f) {
    return s >> f.round >> f.set_id >> f.message;
  }

  struct GrandpaNeighborMessage {
    uint8_t version = 1;
    RoundNumber round_number;
    VoterSetId voter_set_id;
    BlockNumber last_finalized;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s,
                     const GrandpaNeighborMessage &neighbor_message) {
    return s << neighbor_message.version << neighbor_message.round_number
             << neighbor_message.voter_set_id
             << neighbor_message.last_finalized;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaNeighborMessage &neighbor_message) {
    return s >> neighbor_message.version >> neighbor_message.round_number
           >> neighbor_message.voter_set_id >> neighbor_message.last_finalized;
  }

  struct CatchUpRequest {
    RoundNumber round_number;
    VoterSetId voter_set_id;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CatchUpRequest &request) {
    return s << request.round_number << request.voter_set_id;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CatchUpRequest &request) {
    return s >> request.round_number >> request.voter_set_id;
  }

  struct CatchUpResponse {
    VoterSetId voter_set_id{};
    RoundNumber round_number{};
    std::vector<SignedPrevote> prevote_justification;
    std::vector<SignedPrecommit> precommit_justification;
    BlockInfo best_final_candidate;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CatchUpResponse &response) {
    return s << response.voter_set_id << response.round_number
             << response.prevote_justification
             << response.precommit_justification
             << response.best_final_candidate;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CatchUpResponse &response) {
    return s >> response.voter_set_id >> response.round_number
           >> response.prevote_justification >> response.precommit_justification
           >> response.best_final_candidate;
  }

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

#endif  // KAGOME_NETWORK_GRANDPAMESSAGE

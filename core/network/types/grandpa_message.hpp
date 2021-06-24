/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPAMESSAGE
#define KAGOME_NETWORK_GRANDPAMESSAGE

#include <boost/variant.hpp>
#include "consensus/grandpa/structs.hpp"

namespace kagome::network {

  using consensus::grandpa::BlockInfo;
  using consensus::grandpa::Fin;
  using consensus::grandpa::GrandpaJustification;
  using consensus::grandpa::MembershipCounter;
  using consensus::grandpa::RoundNumber;
  using consensus::grandpa::SignedMessage;
  using consensus::grandpa::SignedPrecommit;
  using consensus::grandpa::SignedPrevote;
  using consensus::grandpa::VoteMessage;

  struct GrandpaVote : public VoteMessage {
    using VoteMessage::VoteMessage;
    explicit GrandpaVote(VoteMessage &&vm) noexcept
        : VoteMessage(std::move(vm)){};
  };

  struct GrandpaCommit : public Fin {
    using Fin::Fin;
    explicit GrandpaCommit(Fin &&f) noexcept : Fin(std::move(f)){};
  };

  struct GrandpaNeighborPacket {};

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GrandpaNeighborPacket &request) {
    throw std::runtime_error(
        "Encoding of GrandpaNeighborPacket is not implemented yet");
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaNeighborPacket &request) {
    throw std::runtime_error(
        "Decoding of GrandpaNeighborPacket is not implemented yet");
    return s;
  }

  struct CatchUpRequest {
    RoundNumber round_number;
    MembershipCounter voter_set_id;
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
    MembershipCounter voter_set_id{};
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

  using GrandpaMessage =
      /// Note: order of types in variant matters
      boost::variant<GrandpaVote,            // 0
                     GrandpaCommit,          // 1
                     GrandpaNeighborPacket,  // 2
                     CatchUpRequest,         // 3
                     CatchUpResponse>;       // 4

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPAMESSAGE

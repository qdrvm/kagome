/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_GRANDPAMESSAGE
#define KAGOME_NETWORK_GRANDPAMESSAGE

#include <boost/variant.hpp>
#include "consensus/grandpa/structs.hpp"

namespace {
  using namespace kagome::consensus::grandpa;
}

namespace kagome::network {

  struct GrandpaVoteMessage {};

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GrandpaVoteMessage &request) {
    throw std::runtime_error(
        "Encoding of GrandpaVoteMessage is not implemented yet");
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaVoteMessage &request) {
    throw std::runtime_error(
        "Decoding of GrandpaVoteMessage is not implemented yet");
    return s;
  }

  struct GrandpaPreCommit {};

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GrandpaPreCommit &request) {
    throw std::runtime_error(
        "Encoding of GrandpaPreCommit is not implemented yet");
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaPreCommit &request) {
    throw std::runtime_error(
        "Decoding of GrandpaPreCommit is not implemented yet");
    return s;
  }

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
    size_t voter_set_id;
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
    size_t voter_set_id;
    RoundNumber round_number;
    GrandpaJustification prevote_justification;
    GrandpaJustification precommit_justification;
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
      boost::variant<GrandpaVoteMessage,     // 0
                     GrandpaPreCommit,       // 1
                     GrandpaNeighborPacket,  // 2
                     CatchUpRequest,         // 3
                     CatchUpResponse>;       // 4

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_GRANDPAMESSAGE

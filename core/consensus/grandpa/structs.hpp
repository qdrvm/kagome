/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

#include <boost/asio/steady_timer.hpp>
#include "common/blob.hpp"
#include "common/wrapper.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

  using RoundNumber = common::Wrapper<uint64_t, struct RoundNumberTag>;

  /// voter identifier
  using Id = primitives::AuthorityId;

  /// voter signature
  using Signature = crypto::ED25519Signature;

  /// @tparam Message A protocol message or vote.
  template <typename Message>
  struct SignedMessage {
    Message message;
    Signature signature;
    Id id;
  };

  template <typename Message>
  struct Equivocated {
    Message first;
    Message second;
  };

  namespace detail {
    template <typename Tag>
    struct BlockInfoT {
      primitives::BlockNumber number;
      primitives::BlockHash hash;
    };

    /// Proof of an equivocation (double-vote) in a given round.
    template <typename Message>
    struct Equivocation {  // NOLINT
      /// The round number equivocated in.
      RoundNumber round;
      /// The identity of the equivocator.
      Id id;
      Equivocated<Message> proof;
    };
  }  // namespace detail

  using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;
  using Precommit = detail::BlockInfoT<struct PrecommitTag>;
  using Prevote = detail::BlockInfoT<struct PrevoteTag>;
  using PrimaryPropose = detail::BlockInfoT<struct PrimaryProposeTag>;

  using SignedPrevote = SignedMessage<Prevote>;
  using SignedPrecommit = SignedMessage<Precommit>;
  using SignedPrimaryPropose = SignedMessage<PrimaryPropose>;

  /// A commit message which is an aggregate of precommits.
  struct Commit {
    BlockInfo info;
    std::vector<SignedPrecommit> precommits;
  };

  using PrevoteEquivocation = detail::Equivocation<Prevote>;
  using PrecommitEquivocation = detail::Equivocation<Precommit>;

  struct HistoricalVotes {
    std::vector<SignedPrevote> prevotes_seen;
    std::vector<SignedPrecommit> precommits_seen;
    std::vector<SignedPrimaryPropose> proposes_seen;
    uint64_t prevote_idx;
    uint64_t precommit_idx;
  };

  // TODO(akvinikym) 04.09.19: implement the SCALE codecs
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Precommit &v) {
    return s;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, const Precommit &v) {
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Prevote &v) {
    return s;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, const Prevote &v) {
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const PrimaryPropose &v) {
    return s;
  }
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, const PrimaryPropose &v) {
    return s;
  }
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

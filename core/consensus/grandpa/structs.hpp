/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

#include <boost/asio/steady_timer.hpp>
#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "common/wrapper.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

  using BlockInfo = primitives::BlockInfo;
  using Precommit = primitives::detail::BlockInfoT<struct PrecommitTag>;
  using Prevote = primitives::detail::BlockInfoT<struct PrevoteTag>;
  using PrimaryPropose =
      primitives::detail::BlockInfoT<struct PrimaryProposeTag>;

  const static uint8_t kPrevoteStage = 0;
  const static uint8_t kPrecommitStage = 1;
  const static uint8_t kPrimaryProposeStage = 2;

  /// @tparam Message A protocol message or vote.
  template <typename Message>
  struct SignedMessage {
    Message message;
    Signature signature;
    Id id;
  };

  template <class Stream,
            typename Message,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const SignedMessage<Message> &signed_msg) {
    return s << signed_msg.message << signed_msg.signature << signed_msg.id;
  }

  template <class Stream,
            typename Message,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, SignedMessage<Message> &signed_msg) {
    return s >> signed_msg.message >> signed_msg.signature >> signed_msg.id;
  }

  template <typename Message>
  bool operator==(const SignedMessage<Message> &lhs,
                  const SignedMessage<Message> &rhs) {
    return lhs.message == rhs.message && lhs.signature == rhs.signature
           && lhs.id == rhs.id;
  }

  using SignedPrevote = SignedMessage<Prevote>;
  using SignedPrecommit = SignedMessage<Precommit>;
  using SignedPrimaryPropose = SignedMessage<PrimaryPropose>;

  template <typename Message>
  struct Equivocated {
    Message first;
    Message second;
  };

  namespace detail {
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

  // justification for block B in round r
  struct GrandpaJustification {
    std::vector<SignedPrecommit> items;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GrandpaJustification &v) {
    return s << v.items;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaJustification &v) {
    return s >> v.items;
  }

  /// A commit message which is an aggregate of precommits.
  struct Commit {
    BlockInfo vote;
    GrandpaJustification justification;
  };

  using Vote =
      boost::variant<SignedPrevote,
                     SignedPrecommit,
                     SignedPrimaryPropose>;  // order is important and should
                                             // correspond stage constants
                                             // (kPrevoteStage, kPrecommitStage,
                                             // kPrimaryPropose)

  // either prevote, precommit or primary propose
  struct VoteMessage {
    RoundNumber round_number{0};
    MembershipCounter counter{0};
    Vote vote;

    Id id() const {
      return visit_in_place(
          vote, [](const auto &signed_message) { return signed_message.id; });
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoteMessage &v) {
    return s << v.round_number << v.counter << v.vote;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, VoteMessage &v) {
    return s >> v.round_number >> v.counter >> v.vote;
  }

  // finalizing message or block B in round r
  struct Fin {
    RoundNumber round_number{0};
    BlockInfo vote;
    GrandpaJustification justification;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Fin &f) {
    return s << f.round_number << f.vote << f.justification;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Fin &f) {
    return s >> f.round_number >> f.vote >> f.justification;
  }

  using PrevoteEquivocation = detail::Equivocation<Prevote>;
  using PrecommitEquivocation = detail::Equivocation<Precommit>;

  struct TotalWeight {
    uint64_t prevote;
    uint64_t precommit;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

#include <boost/asio/steady_timer.hpp>
#include <boost/variant.hpp>
#include "common/blob.hpp"
#include "common/wrapper.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

  /// @tparam Message A protocol message or vote.
  template <typename Message>
  struct SignedMessage {
    Message message;
    Signature signature;
    Id id;
  };

  template <typename Message>
  bool operator==(const SignedMessage<Message> &lhs,
                  const SignedMessage<Message> &rhs) {
    return lhs.message == rhs.message && lhs.signature == rhs.signature
           && lhs.id == rhs.id;
  }

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

  using BlockInfo = primitives::BlockInfo;
  using Precommit = primitives::detail::BlockInfoT<struct PrecommitTag>;
  using Prevote = primitives::detail::BlockInfoT<struct PrevoteTag>;
  using PrimaryPropose =
      primitives::detail::BlockInfoT<struct PrimaryProposeTag>;

  using Vote = boost::variant<Prevote, Precommit>;

  using SignedPrevote = SignedMessage<Prevote>;
  using SignedPrecommit = SignedMessage<Precommit>;
  using SignedPrimaryPropose = SignedMessage<PrimaryPropose>;

  // justification for block B in round r
  struct Justification {
    std::vector<SignedPrecommit> items;
  };
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Justification &v) {
    // TODO (kamilsa): implement
    return s;
  }

  /// A commit message which is an aggregate of precommits.
  struct Commit {
    BlockInfo info;
    std::vector<SignedPrecommit> precommits;
  };

  // either prevote, precommit or primary propose
  struct VoteMessage {
    RoundNumber round_number{0};
    MembershipCounter counter{0};
    Id id;
    boost::variant<SignedPrevote, SignedPrecommit, SignedPrimaryPropose> vote;
  };
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoteMessage &v) {
    // TODO (kamilsa): implement
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, const VoteMessage &v) {
    // TODO (kamilsa): implement
    return s;
  }

  // finalizing message or block B in round r
  struct Fin {
    RoundNumber round_number{0};
    BlockInfo vote;
    Justification justification;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Fin &v) {
    // TODO (kamilsa): implement
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator>>(Stream &s, const Fin &v) {
    // TODO (kamilsa): implement
    return s;
  }

  using PrevoteEquivocation = detail::Equivocation<Prevote>;
  using PrecommitEquivocation = detail::Equivocation<Precommit>;

  struct HistoricalVotes {
    std::vector<SignedPrevote> prevotes_seen;
    std::vector<SignedPrecommit> precommits_seen;
    std::vector<SignedPrimaryPropose> proposes_seen;
    uint64_t prevote_idx;
    uint64_t precommit_idx;
  };

  struct TotalWeight {
    uint64_t prevote;
    uint64_t precommit;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

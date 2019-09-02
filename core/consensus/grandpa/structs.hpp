/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

#include <boost/asio/steady_timer.hpp>
#include "common/blob.hpp"
#include "common/wrapper.hpp"

namespace kagome::consensus::grandpa {

  using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

  using BlockHash = common::Wrapper<common::Hash256, struct BlockHashTag>;
  using BlockNumber = common::Wrapper<uint64_t, struct BlockNumberTag>;
  using RoundNumber = common::Wrapper<uint64_t, struct RoundNumberTag>;

  /// voter identifier
  using Id = common::Wrapper<common::Hash256, struct IdTag>;

  /// voter signature
  using Signature = common::Wrapper<std::vector<uint8_t>, struct SignatureTag>;

  namespace detail {
    template <typename Tag>
    struct BlockInfoT {
      BlockNumber number;
      BlockHash hash;
    };

    /// Proof of an equivocation (double-vote) in a given round.
    template <typename Message>
    struct Equivocation {
      /// The round number equivocated in.
      RoundNumber round;
      /// The identity of the equivocator.
      Id id;
      /// The first vote in the equivocation.
      std::pair<Message, Signature> first;
      /// The second vote in the equivocation.
      std::pair<Message, Signature> second;
    };
  }  // namespace detail

  using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;
  using Precommit = detail::BlockInfoT<struct PrecommitTag>;
  using Prevote = detail::BlockInfoT<struct PrevoteTag>;
  using PrimaryPropose = detail::BlockInfoT<struct PrimaryProposeTag>;

  /// @tparam Message A protocol message or vote.
  template <typename Message>
  struct SignedMessage {
    Message message;
    Signature signature;
    Id id;
  };

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
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_STRUCTS_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/container_hash/detail/hash_tuple_like.hpp>
#include <boost/outcome/success_failure.hpp>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "log/logger.hpp"
#include "network/types/collator_messages.hpp"
#include "outcome/custom.hpp"
#include "parachain/types.hpp"

/// References
/// https://github.com/paritytech/polkadot-sdk/blob/157294b0d39f1b3dd7307a70c77de5267134ede2/polkadot/node/network/statement-distribution/src/v2/cluster.rs
///
/// Direct distribution of statements within a cluster,
/// even those concerning candidates which are not yet backed.
///
/// Members of a validation group assigned to a para at a given relay-parent
/// always distribute statements directly to each other.
///
/// The main way we limit the amount of candidates that have to be handled by
/// the system is to limit the amount of `Seconded` messages that we allow
/// each validator to issue at each relay-parent. Since the amount of
/// relay-parents that we have to deal with at any time is itself bounded, this
/// lets us bound the memory and work that we have here. Bounding `Seconded`
/// statements is enough because they imply a bounded amount of `Valid`
/// statements about the same candidate which may follow.
///
/// The motivation for this piece of code is that the statements that each
/// validator sees may differ. i.e. even though a validator is allowed to issue
/// X `Seconded` statements at a relay-parent, they may in fact issue X*2 and
/// issue one set to one partition of the backing group and one set to another.
/// Of course, in practice these types of partitions will not exist, but in the
/// worst case each validator in the group would see an entirely different set
/// of X `Seconded` statements from some validator and each validator is in its
/// own partition. After that partition resolves, we'd have to deal with up to
/// `limit*group_size` `Seconded` statements from that validator. And then if
/// every validator in the group does the same thing, we're dealing with
/// something like `limit*group_size^2` `Seconded` statements in total.
///
/// Given that both our group sizes and our limits per relay-parent are small,
/// this is quite manageable, and the utility here lets us deal with it in only
/// a few kilobytes of memory.
///
/// It's also worth noting that any case where a validator issues more than the
/// legal limit of `Seconded` statements at a relay parent is trivially
/// slashable on-chain, which means the 'worst case' adversary that this code
/// defends against is effectively lighting money on fire. Nevertheless, we
/// handle the case here to ensure that the behavior of the system is
/// well-defined even if an adversary is willing to be slashed.
///
/// More concretely, this module exposes a [`ClusterTracker`] utility which
/// allows us to determine whether to accept or reject messages from other
/// validators in the same group as we are in, based on _the most charitable
/// possible interpretation of our protocol rules_, and to keep track of what we
/// have sent to other validators in the group and what we may continue to send
/// them.

namespace kagome::parachain {

  struct GeneralKnowledge {
    SCALE_TIE(1);

    CandidateHash hash;
  };

  // Specific knowledge of a given statement (with its originator)
  struct SpecificKnowledge {
    SCALE_TIE(2);

    network::CompactStatement statement;
    ValidatorIndex index;
  };

  // A piece of knowledge about a candidate
  using Knowledge = std::variant<GeneralKnowledge, SpecificKnowledge>;

  struct IncomingP2P {
    SCALE_TIE(1);

    Knowledge knowledge;
  };

  struct OutgoingP2P {
    SCALE_TIE(1);

    Knowledge knowledge;
  };
  struct Seconded {
    SCALE_TIE(1);

    CandidateHash hash;
  };

  using TaggedKnowledge = std::variant<IncomingP2P, OutgoingP2P, Seconded>;

  enum class Accept {
    /// Neither the peer nor the originator have apparently exceeded limits.
    /// Candidate or statement may already be known.
    Ok,
    /// Accept the message; the peer hasn't exceeded limits but the originator
    /// has.
    WithPrejudice,
  };

  /// Incoming statement was rejected.
  enum class RejectIncoming {
    /// Peer sent excessive `Seconded` statements.
    ExcessiveSeconded,
    /// Sender or originator is not in the group.
    NotInGroup,
    /// Candidate is unknown to us. Only applies to `Valid` statements.
    CandidateUnknown,
    /// Statement is duplicate.
    Duplicate,
  };

  /// Outgoing statement was rejected.
  enum class RejectOutgoing {
    /// Candidate was unknown. Only applies to `Valid` statements.
    CandidateUnknown,
    /// We attempted to send excessive `Seconded` statements.
    /// indicates a bug on the local node's code.
    ExcessiveSeconded,
    /// The statement was already known to the peer.
    Known,
    /// Target or originator not in the group.
    NotInGroup,
  };
}  // namespace kagome::parachain

template <>
struct std::hash<kagome::parachain::GeneralKnowledge> {
  auto operator()(const kagome::parachain::GeneralKnowledge &v) {
    return std::hash<kagome::parachain::CandidateHash>{}(v.hash);
  }
};

template <>
struct std::hash<kagome::parachain::SpecificKnowledge> {
  auto operator()(const kagome::parachain::SpecificKnowledge &v) {
    return boost::hash_value(std::forward_as_tuple(v.index, v.statement));
  }
};

template <>
struct std::hash<kagome::parachain::IncomingP2P> {
  auto operator()(const kagome::parachain::IncomingP2P &v) {
    return std::hash<kagome::parachain::Knowledge>{}(v.knowledge);
  }
};

template <>
struct std::hash<kagome::parachain::OutgoingP2P> {
  auto operator()(const kagome::parachain::OutgoingP2P &v) {
    return std::hash<kagome::parachain::Knowledge>{}(v.knowledge);
  }
};

template <>
struct std::hash<kagome::parachain::Seconded> {
  auto operator()(const kagome::parachain::Seconded &v) {
    return std::hash<kagome::parachain::CandidateHash>{}(v.hash);
  }
};

namespace kagome::parachain {
  class ClusterTracker {
   public:
    ClusterTracker(std::vector<ValidatorIndex> validators,
                   size_t seconding_limit);

    CustomOutcome<Accept, RejectIncoming> can_receive(
        ValidatorIndex sender,
        ValidatorIndex originator,
        network::CompactStatement statement);

    /// Dumps pending statement for this cluster.
    ///
    /// Normally we should not have pending statements to validators in our
    /// cluster, but if we do for all validators in our cluster, then we don't
    /// participate in backing. Occasional pending statements are expected if
    /// two authorities can't detect each other or after restart, where it takes
    /// a while to discover the whole network.
    void warn_if_too_many_pending_statements(const common::Hash256 &hash);

    /// Note that we issued a statement. This updates internal structures.
    void note_issued(ValidatorIndex originator,
                     network::CompactStatement statement);

    /// Note that we accepted an incoming statement. This updates internal
    /// structures.
    ///
    /// Should only be called after a successful `can_receive` call.
    void note_received(ValidatorIndex sender,
                       ValidatorIndex originator,
                       network::CompactStatement statement);

    CustomOutcome<void, RejectOutgoing> can_send(
        ValidatorIndex target,
        ValidatorIndex originator,
        network::CompactStatement statement);

    /// Note that we sent an outgoing statement to a peer in the group.
    /// This must be preceded by a successful `can_send` call.
    void note_sent(ValidatorIndex target,
                   ValidatorIndex originator,
                   network::CompactStatement statement);

    std::span<const ValidatorIndex> targets() const;

    std::span<const ValidatorIndex> senders_for_originator(
        ValidatorIndex originator) const;
    /// Returns a Vec of pending statements to be sent to a particular validator
    /// index. `Seconded` statements are sorted to the front of the vector.
    ///
    /// Pending statements have the form (originator, compact statement).
    std::vector<std::pair<ValidatorIndex, network::CompactStatement>>
    pending_statements_for(ValidatorIndex target) const;

    bool knows_candidate(ValidatorIndex validator,
                         CandidateHash candidate_hash) const;
    bool can_request(ValidatorIndex target, CandidateHash candidate_hash) const;

   private:
    // returns true if it's legal to accept a new `Seconded` message from this
    // validator. This is either
    //   1. because we've already accepted it.
    //   2. because there's space for more seconding.
    bool seconded_already_or_within_limit(ValidatorIndex validator,
                                          CandidateHash candidate_hash) const;
    bool they_know_statement(ValidatorIndex validator,
                             ValidatorIndex originator,
                             network::CompactStatement statement) const;

    bool they_sent(ValidatorIndex validator, Knowledge knowledge) const;

    bool we_sent(ValidatorIndex validator, Knowledge knowledge) const;

    bool we_sent_seconded(ValidatorIndex validator,
                          CandidateHash candidate_hash) const;

    bool they_sent_seconded(ValidatorIndex validator,
                            CandidateHash candidate_hash) const;
    bool validator_seconded(ValidatorIndex validator,
                            CandidateHash candidate_hash) const;
    bool is_in_group(ValidatorIndex validator) const;

    std::vector<ValidatorIndex> validators;
    log::Logger logger_ = log::createLogger("ClusterTracker", "parachain");
    size_t seconding_limit;
    std::unordered_map<ValidatorIndex, std::unordered_set<TaggedKnowledge>>
        knowledge;

    // Statements known locally which haven't been sent to particular
    // validators. maps target validator to (originator, statement) pairs.
    std::unordered_map<
        ValidatorIndex,
        std::unordered_set<
            std::pair<ValidatorIndex, network::CompactStatement>,
            boost::hash<std::pair<ValidatorIndex, network::CompactStatement>>>>
        pending;
  };

}  // namespace kagome::parachain

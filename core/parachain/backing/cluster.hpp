/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "network/types/collator_messages.hpp"
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
    CandidateHash hash;
  };

  // Specific knowledge of a given statement (with its originator)
  struct SpecificKnowledge {
    network::CompactStatement statement;
    ValidatorIndex index;
  };

  // A piece of knowledge about a candidate
  using Knowledge = std::variant<GeneralKnowledge, SpecificKnowledge>;

  struct TaggedKnowledge {
    enum Tag {
      // Knowledge we have received from the validator on the p2p layer.
      IncomingP2P,
      // Knowledge we have sent to the validator on the p2p layer.
      OutgoingP2P,
      // Knowledge of candidates the validator has seconded.
      // This is limited only to `Seconded` statements we have accepted
      // _without prejudice_.
      Seconded,
    } tag;
    Knowledge knowledge;
    CandidateHash hash;
  };

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

  class ClusterTracker {
   public:
    ClusterTracker(std::vector<ValidatorIndex> validators,
                   size_t seconding_limit)
        : validators{std::move(validators)},
          seconding_limit{seconding_limit},
          knowledge{},
          pending{} {}

    outcome::result<Accept, RejectIncoming> can_receive(
        ValidatorIndex sender,
        ValidatorIndex originator,
        network::CompactStatement statement) {
      if (!is_in_group(sender) || !is_in_group(originator)) {
        return RejectIncoming::NotInGroup;
      }

      if (they_sent(sender, SpecificKnowledge{statement, originator})) {
        return RejectIncoming::Duplicate;
      }

      if (auto seconded =
              std::get_if<network::CompactStatementSeconded>(&statement)) {
        // check whether the sender has not sent too many seconded statements
        // for the originator. we know by the duplicate check above that this
        // iterator doesn't include the statement itself.

        if (auto it = knowledge.find(sender); it != knowledge.end()) {
          auto &knowledge_set = it->second;
          auto other_seconded_for_orig_from_remote =
              knowledge_set | std::views::join()
              | std::views::filter([](const TaggedKnowledge &knowledge) {
                  if (knowledge.tag == TaggedKnowledge::IncomingP2P) {
                    if (auto *specific_knowledge =
                            std::get_if<SpecificKnowledge>(
                                &knowledge.knowledge)) {
                      if (auto *seconeded_stmt =
                              std::get_if<network::CompactStatementSeconded>(
                                  &specific_knowledge->statement)) {
                      }
                    }
                  }
                });
        }
      }
    }

   private:
    bool is_in_group(ValidatorIndex validator) const {
      return std::ranges::find(validators, validator) != validators.end();
    }

    std::vector<ValidatorIndex> validators;
    size_t seconding_limit;
    std::unordered_map<ValidatorIndex, std::unordered_set<TaggedKnowledge>>
        knowledge;
    // Statements known locally which haven't been sent to particular
    // validators. maps target validator to (originator, statement) pairs.
    std::unordered_map<ValidatorIndex,
                       std::unordered_set<std::pair<ValidatorIndex,
                                                    network::CompactStatement>>>
        pending;
  };

}  // namespace kagome::parachain
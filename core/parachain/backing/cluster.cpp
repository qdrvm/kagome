/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/backing/cluster.hpp"

namespace kagome::parachain {

  ClusterTracker::ClusterTracker(std::vector<ValidatorIndex> validators,
                                 size_t seconding_limit)
      : validators{std::move(validators)},
        seconding_limit{seconding_limit},
        knowledge{},
        pending{} {}

  outcome::result<Accept, RejectIncoming> ClusterTracker::can_receive(
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

      size_t other_seconded_for_orig_from_remote{};
      if (auto it = knowledge.find(sender); it != knowledge.end()) {
        auto &knowledge_set = it->second;

        for (auto &tagged_knowledge : knowledge_set) {
          if (auto *incoming = std::get_if<IncomingP2P>(&tagged_knowledge)) {
            if (auto *specific_knowledge =
                    std::get_if<SpecificKnowledge>(&incoming->knowledge)) {
              if (auto *seconeded_stmt =
                      std::get_if<network::CompactStatementSeconded>(
                          &specific_knowledge->statement)) {
                if (specific_knowledge->index == originator) {
                  other_seconded_for_orig_from_remote++;
                }
              }
            }
          }
        }
      }
      if (other_seconded_for_orig_from_remote == seconding_limit) {
        return RejectIncoming::ExcessiveSeconded;
      }
      if (seconded_already_or_within_limit(originator, *seconded)) {
        return Accept::Ok;
      }
      return Accept::WithPrejudice;
    }
    if (auto valid = std::get_if<network::CompactStatementValid>(&statement)) {
      if (!knows_candidate(sender, *valid)) {
        return RejectIncoming::CandidateUnknown;
      }
      return Accept::Ok;
    }
    static_assert(std::variant_size_v<network::CompactStatement> == 2);
    UNREACHABLE;
  }

  void ClusterTracker::warn_if_too_many_pending_statements(
      const common::Hash256 &hash) {
    size_t pending_count{};
    for (auto &pending_set : pending) {
      if (!pending_set.second.empty()) {
        pending_count++;
      }
    }

    if (pending_count >= validators.size()) {
      SL_WARN(
          logger_,
          "Cluster has too many pending statements, something wrong with our connection to our group peers. "
          "Restart might be needed if validator gets 0 backing rewards for more than 3-4 consecutive sessions");
    }
  }

  void ClusterTracker::note_issued(ValidatorIndex originator,
                                   network::CompactStatement statement) {
    for (auto &cluster_member : validators) {
      if (!they_know_statement(cluster_member, originator, statement)) {
        // add the statement to pending knowledge for all peers
        // which don't know the statement.
        pending[cluster_member].insert({originator, statement});
      }
    }
  }

  void ClusterTracker::note_received(ValidatorIndex sender,
                                     ValidatorIndex originator,
                                     network::CompactStatement statement) {
    for (auto &cluster_member : validators) {
      if (cluster_member == sender) {
        if (auto it = pending.find(sender); it != pending.end()) {
          it->second.erase({originator, statement});
        }
      } else if (!they_know_statement(cluster_member, originator, statement)) {
        pending[cluster_member].insert({originator, statement});
      }
    }
    knowledge[sender].insert(
        IncomingP2P{SpecificKnowledge{statement, originator}});
    if (auto *seconded =
            std::get_if<network::CompactStatementSeconded>(&statement)) {
      knowledge[sender].insert(IncomingP2P{GeneralKnowledge{*seconded}});
      // since we accept additional `Seconded` statements beyond the limits
      // 'with prejudice', we must respect the limit here.
      if (seconded_already_or_within_limit(originator, *seconded)) {
        knowledge[originator].insert(Seconded{*seconded});
      }
    }
  }

  outcome::result<void, RejectOutgoing> ClusterTracker::can_send(
      ValidatorIndex target,
      ValidatorIndex originator,
      network::CompactStatement statement) {
    if (!is_in_group(target) || !is_in_group(originator)) {
      return RejectOutgoing::NotInGroup;
    }
    if (they_know_statement(target, originator, statement)) {
      return RejectOutgoing::Known;
    }
    if (auto *seconded =
            std::get_if<network::CompactStatementSeconded>(&statement)) {
      // we send the same `Seconded` statements to all our peers, and only the
      // first `k` from each originator.
      if (!seconded_already_or_within_limit(originator, *seconded)) {
        return RejectOutgoing::ExcessiveSeconded;
      }
    } else if (auto *valid =
                   std::get_if<network::CompactStatementValid>(&statement)) {
      if (!knows_candidate(target, *valid)) {
        return RejectOutgoing::CandidateUnknown;
      }
    }
    return outcome::success();
  }

  void ClusterTracker::note_sent(ValidatorIndex target,
                                 ValidatorIndex originator,
                                 network::CompactStatement statement) {
    knowledge[target].insert(
        OutgoingP2P{SpecificKnowledge{statement, originator}});
    if (auto *seconded =
            std::get_if<network::CompactStatementSeconded>(&statement)) {
      knowledge[target].insert(OutgoingP2P{GeneralKnowledge{*seconded}});
      knowledge[originator].insert(Seconded{*seconded});
    }
    if (auto it = pending.find(target); it != pending.end()) {
      it->second.erase({originator, statement});
    }
  }

  std::span<const ValidatorIndex> ClusterTracker::targets() const {
    return validators;
  }

  std::span<const ValidatorIndex> ClusterTracker::senders_for_originator(
      ValidatorIndex originator) const {
    if (std::ranges::find(validators, originator) != std::end(validators)) {
      return validators;
    }
    return {};
  }

  std::vector<std::pair<ValidatorIndex, network::CompactStatement>>
  ClusterTracker::pending_statements_for(ValidatorIndex target) const {
    auto pending_set_it = pending.find(target);
    if (pending_set_it == pending.end()) {
      return {};
    }
    auto &pending_set = pending_set_it->second;
    std::vector<std::pair<ValidatorIndex, network::CompactStatement>> result(
        pending_set.size());
    std::ranges::copy(pending_set, result.begin());
    std::ranges::sort(result, [](auto &idx_stmt_1, auto &idx_stmt_2) {
      return idx_stmt_1.second.index() < idx_stmt_2.second.index();
    });
    return result;
  }

  bool ClusterTracker::knows_candidate(ValidatorIndex validator,
                                       CandidateHash candidate_hash) const {
    // we sent, they sent, or they signed and we received from someone else.
    return we_sent_seconded(validator, candidate_hash)
        || they_sent_seconded(validator, candidate_hash)
        || validator_seconded(validator, candidate_hash);
  }

  bool ClusterTracker::can_request(ValidatorIndex target,
                                   CandidateHash candidate_hash) const {
    return std::ranges::find(validators, target) != std::end(validators)
        && we_sent_seconded(target, candidate_hash)
        && !they_sent_seconded(target, candidate_hash);
  }

  bool ClusterTracker::seconded_already_or_within_limit(
      ValidatorIndex validator, CandidateHash candidate_hash) const {
    size_t seconded_other_candidates = 0;

    auto knowledge_set_it = knowledge.find(validator);
    if (knowledge_set_it != knowledge.end()) {
      auto &knowledge_set = knowledge_set_it->second;
      for (auto &knowledge : knowledge_set) {
        if (auto *seconded = std::get_if<Seconded>(&knowledge)) {
          if (seconded->hash != candidate_hash) {
            seconded_other_candidates++;
          }
        }
      }
    }

    // This fulfills both properties by under-counting when the validator is
    // at the limit but _has_ seconded the candidate already.
    return seconded_other_candidates < seconding_limit;
  }

  bool ClusterTracker::they_know_statement(
      ValidatorIndex validator,
      ValidatorIndex originator,
      network::CompactStatement statement) const {
    SpecificKnowledge knowledge{statement, originator};
    return we_sent(validator, knowledge) || they_sent(validator, knowledge);
  }

  bool ClusterTracker::they_sent(ValidatorIndex validator,
                                 Knowledge knowledge) const {
    auto knowledge_set = this->knowledge.find(validator);
    if (knowledge_set == this->knowledge.end()) {
      return false;
    }
    return knowledge_set->second.contains(IncomingP2P{knowledge});
  }

  bool ClusterTracker::we_sent(ValidatorIndex validator,
                               Knowledge knowledge) const {
    auto knowledge_set = this->knowledge.find(validator);
    if (knowledge_set == this->knowledge.end()) {
      return false;
    }
    return knowledge_set->second.contains(OutgoingP2P{knowledge});
  }

  bool ClusterTracker::we_sent_seconded(ValidatorIndex validator,
                                        CandidateHash candidate_hash) const {
    return we_sent(validator, GeneralKnowledge{candidate_hash});
  }

  bool ClusterTracker::they_sent_seconded(ValidatorIndex validator,
                                          CandidateHash candidate_hash) const {
    return they_sent(validator, GeneralKnowledge{candidate_hash});
  }

  bool ClusterTracker::validator_seconded(ValidatorIndex validator,
                                          CandidateHash candidate_hash) const {
    auto knowledge_set = knowledge.find(validator);
    if (knowledge_set == knowledge.end()) {
      return false;
    }
    return knowledge_set->second.contains(Seconded{candidate_hash});
  }

  bool ClusterTracker::is_in_group(ValidatorIndex validator) const {
    return std::ranges::find(validators, validator) != validators.end();
  }

}  // namespace kagome::parachain

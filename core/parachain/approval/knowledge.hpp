/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <tuple>

#include "common/visitor.hpp"
#include "consensus/timeline/types.hpp"
#include "outcome/outcome.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/approval/state.hpp"
#include "parachain/types.hpp"

template <>
struct std::hash<scale::BitVec> {
  auto operator()(const scale::BitVec &v) const {
    auto s = scale::encode(v).value();
    return boost::hash_range(s.begin(), s.end());
  }
};

namespace kagome::parachain::approval {

  enum struct MessageKind { Assignment, Approval };
  using MessageSubject = std::tuple<Hash, scale::BitVec, ValidatorIndex>;

  struct MessageSubjectHash {
    auto operator()(const MessageSubject &obj) const {
      size_t value{0ull};
      boost::hash_range(
          value, std::get<0>(obj).begin(), std::get<0>(obj).end());
      boost::hash_range(
          value, std::get<1>(obj).bits.begin(), std::get<1>(obj).bits.end());
      boost::hash_combine(value, std::get<2>(obj));
      return value;
    }
  };

  struct Knowledge {
    // When there is no entry, this means the message is unknown
    // When there is an entry with `MessageKind::Assignment`, the assignment is
    // known. When there is an entry with `MessageKind::Approval`, the
    // assignment and approval are known.
    std::unordered_map<MessageSubject, MessageKind, MessageSubjectHash>
        known_messages{};

    bool can_send(const MessageSubject &message,
      const MessageKind &kind) const {
        auto it = known_messages.find(message);
        if (it == known_messages.end()) {
          return MessageKind::Assignment == kind;
        }

        return (kind == MessageKind::Approval && it->second == MessageKind::Assignment);
    }

    bool contains(const MessageSubject &message,
                  const MessageKind &kind) const {
      auto it = known_messages.find(message);
      if (it == known_messages.end()) {
        return false;
      }
      if (MessageKind::Assignment == kind) {
        return true;
      }
      return MessageKind::Approval == it->second;
    }

    bool insert(const MessageSubject &message, const MessageKind &kind) {
      const auto &[it, inserted] = known_messages.emplace(message, kind);
      if (inserted) {
        return true;
      }
      if (MessageKind::Assignment == it->second
          && MessageKind::Approval == kind) {
        it->second = MessageKind::Approval;
        return true;
      }
      return false;
    }
  };

  struct PeerKnowledge {
    /// The knowledge we've sent to the peer.
    Knowledge sent{};
    /// The knowledge we've received from the peer.
    Knowledge received{};

    bool contains(const MessageSubject &message,
                  const MessageKind &kind) const {
      return sent.contains(message, kind) || received.contains(message, kind);
    }

    // Generate the knowledge keys for querying if an approval is known by peer.
    static std::pair<MessageSubject, MessageKind> generate_approval_key(
        const IndirectSignedApprovalVoteV2 &approval) {
      return {
          std::make_tuple(approval.payload.payload.block_hash,
                          approval.payload.payload.candidate_indices,
                          approval.payload.ix),
          MessageKind::Approval,
      };
    }
  };

}  // namespace kagome::parachain::approval

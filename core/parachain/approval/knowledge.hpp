/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KNOWLEDGE_HPP
#define KAGOME_KNOWLEDGE_HPP

#include <tuple>

#include "common/visitor.hpp"
#include "consensus/timeline/types.hpp"
#include "consensus/validation/prepare_transcript.hpp"
#include "outcome/outcome.hpp"
#include "parachain/approval/state.hpp"
#include "parachain/types.hpp"

namespace kagome::parachain::approval {

  enum struct MessageKind { Assignment, Approval };
  using MessageSubject = std::tuple<Hash, CandidateIndex, ValidatorIndex>;

  struct MessageSubjectHash {
    auto operator()(const MessageSubject &obj) const {
      size_t value{0ull};
      std::apply(
          [&](const auto &...v) { (..., boost::hash_combine(value, v)); }, obj);
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
  };

}  // namespace kagome::parachain::approval

#endif  // KAGOME_KNOWLEDGE_HPP

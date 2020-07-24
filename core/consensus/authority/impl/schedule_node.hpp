/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#ifndef KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE
#define KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

#include "primitives/authority.hpp"

namespace kagome::authority {

  class ScheduleNode : public std::enable_shared_from_this<ScheduleNode> {
   public:
    ScheduleNode() = default;

    ScheduleNode(const std::shared_ptr<ScheduleNode> &ancestor,
                 primitives::BlockInfo block)
        : block(std::move(block)), parent(ancestor) {
      BOOST_ASSERT((bool)ancestor);
    }

    outcome::result<void> ensureReadyToSchedule() const;

    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block);

    const primitives::BlockInfo block{};
    std::weak_ptr<ScheduleNode> parent;
    std::vector<std::shared_ptr<ScheduleNode>> descendants{};

    // Current authorities
    std::shared_ptr<const primitives::AuthorityList> actual_authorities;
    bool enabled = true;

    static constexpr auto inactive =
        std::numeric_limits<primitives::BlockNumber>::max();

    // For scheduled changes
    primitives::BlockNumber scheduled_after = inactive;
    std::shared_ptr<const primitives::AuthorityList> scheduled_authorities{};

    // For forced changed
    primitives::BlockNumber forced_for = inactive;
    std::shared_ptr<const primitives::AuthorityList> forced_authorities{};

    // For pause
    primitives::BlockNumber pause_after = inactive;

    // For resume
    primitives::BlockNumber resume_for = inactive;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ScheduleNode &b) {
    s << b.block << b.actual_authorities << b.enabled;
    if (b.scheduled_after != ScheduleNode::inactive) {
      s << b.scheduled_after << b.scheduled_authorities;
    } else {
      s << 0;
    }
    if (b.forced_for != ScheduleNode::inactive) {
      s << b.forced_for << b.forced_authorities;
    } else {
      s << 0;
    }
    if (b.pause_after != ScheduleNode::inactive) {
      s << b.pause_after;
    } else {
      s << 0;
    }
    if (b.resume_for != ScheduleNode::inactive) {
      s << b.resume_for;
    } else {
      s << 0;
    }
    s << b.descendants;
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ScheduleNode &b) {
    s >> const_cast<primitives::BlockInfo &>(b.block)  // NOLINT
        >> b.actual_authorities >> b.enabled;
    primitives::BlockNumber bn;
    if (s >> bn, bn) {
      b.scheduled_after = bn;
      s >> b.scheduled_authorities;
    } else {
      b.scheduled_after = ScheduleNode::inactive;
    }
    if (s >> bn, bn) {
      b.forced_for = bn;
      s >> b.forced_authorities;
    } else {
      b.scheduled_after = ScheduleNode::inactive;
    }
    if (s >> bn, bn) {
      b.pause_after = bn;
    } else {
      b.pause_after = ScheduleNode::inactive;
    }
    if (s >> bn, bn) {
      b.resume_for = bn;
    } else {
      b.resume_for = ScheduleNode::inactive;
    }
    s >> b.descendants;
    return s;
  }

}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

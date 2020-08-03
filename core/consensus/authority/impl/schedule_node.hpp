/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE
#define KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

#include "primitives/authority.hpp"

namespace kagome::authority {

  /**
   * @brief Node of scheduler tree. Contains actual authorities for the accorded
   * block and all its descendant blocks until any changes are applied. May
   * contain one of the changes.
   */
  class ScheduleNode : public std::enable_shared_from_this<ScheduleNode> {
   public:
    ScheduleNode() = default;

    ScheduleNode(const std::shared_ptr<ScheduleNode> &ancestor,
                 primitives::BlockInfo block);

    static std::shared_ptr<ScheduleNode> createAsRoot(
        primitives::BlockInfo block);

    outcome::result<void> ensureReadyToSchedule() const;

    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block, bool finalized = false);

    const primitives::BlockInfo block{};
    std::weak_ptr<ScheduleNode> parent;
    std::vector<std::shared_ptr<ScheduleNode>> descendants{};

    // Current authorities
    std::shared_ptr<const primitives::AuthorityList> actual_authorities;
    bool enabled = true;

    static constexpr auto INACTIVE = 0;

    // For scheduled changes
    primitives::BlockNumber scheduled_after = INACTIVE;
    std::shared_ptr<const primitives::AuthorityList> scheduled_authorities{};

    // For forced changed
    primitives::BlockNumber forced_for = INACTIVE;
    std::shared_ptr<const primitives::AuthorityList> forced_authorities{};

    // For pause
    primitives::BlockNumber pause_after = INACTIVE;

    // For resume
    primitives::BlockNumber resume_for = INACTIVE;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ScheduleNode &b) {
    s << b.block << b.actual_authorities << b.enabled;
    if (b.scheduled_after != ScheduleNode::INACTIVE) {
      s << b.scheduled_after << b.scheduled_authorities;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.forced_for != ScheduleNode::INACTIVE) {
      s << b.forced_for << b.forced_authorities;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.pause_after != ScheduleNode::INACTIVE) {
      s << b.pause_after;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.resume_for != ScheduleNode::INACTIVE) {
      s << b.resume_for;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
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
      b.scheduled_after = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.forced_for = bn;
      s >> b.forced_authorities;
    } else {
      b.forced_for = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.pause_after = bn;
    } else {
      b.pause_after = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.resume_for = bn;
    } else {
      b.resume_for = ScheduleNode::INACTIVE;
    }
    s >> b.descendants;
    return s;
  }

}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

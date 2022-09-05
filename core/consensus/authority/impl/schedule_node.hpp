/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE
#define KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

#include "common/tagged.hpp"
#include "primitives/authority.hpp"

namespace kagome::authority {

  using IsBlockFinalized = Tagged<bool, struct IsBlockFinalizedTag>;

  /**
   * @brief Node of scheduler tree. Contains actual authorities for the accorded
   * block and all its descendant blocks until any changes are applied. May
   * contain one of the changes.
   */
  class ScheduleNode : public std::enable_shared_from_this<ScheduleNode> {
   public:
    static constexpr primitives::BlockNumber INACTIVE = 0;

    ScheduleNode() = default;

    ScheduleNode(const std::shared_ptr<const ScheduleNode> &ancestor,
                 primitives::BlockInfo block);

    /// Creates schedule node as root
    /// @result schedule node
    static std::shared_ptr<ScheduleNode> createAsRoot(
        primitives::BlockInfo block);

    /// Ensure if all significant changes is applied and node is ready to
    /// schedule next change
    /// @result success or error
    outcome::result<void> ensureReadyToSchedule() const;

    /// Changes actual authorities as if corresponding block is finalized or not
    /// @param finalized - true if block is finalized
    void adjust(IsBlockFinalized finalized);

    /// Creates descendant schedule node for block
    /// @param block - target block
    /// @param finalized - true if block is finalized
    /// @result schedule node
    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block, IsBlockFinalized finalized) const;

    const primitives::BlockInfo block{};
    std::weak_ptr<const ScheduleNode> parent;
    std::vector<std::shared_ptr<ScheduleNode>> descendants{};

    // Current authorities
    std::shared_ptr<const primitives::AuthorityList> actual_authorities;
    bool enabled = true;

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
    s << b.block << b.actual_authorities << b.actual_authorities->id
      << b.enabled;
    if (b.scheduled_after != ScheduleNode::INACTIVE) {
      s << b.scheduled_after << b.scheduled_authorities
        << b.scheduled_authorities->id;
    } else {
      s << static_cast<primitives::BlockNumber>(0);
    }
    if (b.forced_for != ScheduleNode::INACTIVE) {
      s << b.forced_for << b.forced_authorities << b.forced_authorities->id;
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
    s >> const_cast<primitives::BlockInfo &>(b.block);  // NOLINT
    s >> b.actual_authorities;
    s >> const_cast<uint64_t &>(b.actual_authorities->id);  // NOLINT
    s >> b.enabled;
    primitives::BlockNumber bn;
    if (s >> bn, bn) {
      b.scheduled_after = bn;
      s >> b.scheduled_authorities;
      s >> const_cast<uint64_t &>(b.scheduled_authorities->id);  // NOLINT
    } else {
      b.scheduled_after = ScheduleNode::INACTIVE;
    }
    if (s >> bn, bn) {
      b.forced_for = bn;
      s >> b.forced_authorities;
      s >> const_cast<uint64_t &>(b.forced_authorities->id);  // NOLINT
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

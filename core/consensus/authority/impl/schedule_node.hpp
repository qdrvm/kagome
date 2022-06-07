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
    void adjust(bool finalized);

    /// Creates descendant schedule node for block
    /// @param block - target block
    /// @param finalized - true if block is finalized
    /// @result schedule node
    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block, bool finalized) const;

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

}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

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
    ScheduleNode(const std::shared_ptr<ScheduleNode> &ancestor,
                 primitives::BlockInfo block)
        : block(std::move(block)), parent(ancestor) {
      BOOST_ASSERT((bool)ancestor);
    }

    outcome::result<void> ensureReadyToSchedule() const;

    std::shared_ptr<ScheduleNode> makeDescendant(
        const primitives::BlockInfo &block);

    const primitives::BlockInfo block;
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

}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_SCHEDULE_NODE

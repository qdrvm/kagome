/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"

namespace kagome::authority {

  ScheduleNode::ScheduleNode(
      const std::shared_ptr<const ScheduleNode> &ancestor,
      primitives::BlockInfo block)
      : block(block), parent(ancestor) {
    BOOST_ASSERT((bool)ancestor);
  }

  std::shared_ptr<ScheduleNode> ScheduleNode::createAsRoot(
      primitives::BlockInfo block) {
    auto fake_parent = std::make_shared<ScheduleNode>(ScheduleNode());
    return std::make_shared<ScheduleNode>(fake_parent, block);
  }

  outcome::result<void> ScheduleNode::ensureReadyToSchedule() const {
    if (scheduled_after != INACTIVE) {
      return AuthorityUpdateObserverError::NO_SCHEDULED_CHANGE_APPLIED_YET;
    }
    if (forced_for != INACTIVE) {
      return AuthorityUpdateObserverError::NO_FORCED_CHANGE_APPLIED_YET;
    }
    if (pause_after != INACTIVE) {
      return AuthorityUpdateObserverError::NO_PAUSE_APPLIED_YET;
    }
    if (resume_for != INACTIVE) {
      return AuthorityUpdateObserverError::NO_RESUME_APPLIED_YET;
    }
    return outcome::success();
  }

  void ScheduleNode::adjust(IsBlockFinalized finalized) {
    // Has ScheduledChange
    if (scheduled_after != INACTIVE) {
      if (finalized && scheduled_after <= block.number) {
        actual_authorities = std::move(scheduled_authorities);
        scheduled_after = INACTIVE;
      }
    }
    // Has ForcedChange
    else if (forced_for != INACTIVE) {
      if (forced_for <= block.number) {
        actual_authorities = std::move(forced_authorities);
        forced_for = INACTIVE;
      }
    }
    // Has planned pause
    else if (pause_after != INACTIVE) {
      if (finalized && pause_after <= block.number) {
        enabled = false;
        pause_after = INACTIVE;
      }
    }
    // Has planned resume
    else if (resume_for != INACTIVE) {
      if (resume_for <= block.number) {
        enabled = true;
        resume_for = INACTIVE;
      }
    }
  }

  std::shared_ptr<ScheduleNode> ScheduleNode::makeDescendant(
      const primitives::BlockInfo &target_block,
      IsBlockFinalized finalized) const {
    auto node =
        std::make_shared<ScheduleNode>(shared_from_this(), target_block);
    node->actual_authorities = actual_authorities;
    node->enabled = enabled;
    node->forced_for = forced_for;
    node->forced_authorities = forced_authorities;
    node->scheduled_after = scheduled_after;
    node->scheduled_authorities = scheduled_authorities;
    node->pause_after = pause_after;
    node->resume_for = resume_for;
    node->adjust(finalized);
    return node;
  }
}  // namespace kagome::authority

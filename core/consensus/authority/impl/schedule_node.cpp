/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"

namespace kagome::authority {

  ScheduleNode::ScheduleNode(const std::shared_ptr<const ScheduleNode> &ancestor,
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

  std::shared_ptr<ScheduleNode> ScheduleNode::makeDescendant(
      const primitives::BlockInfo &target_block, bool finalized) const {
    auto node = std::make_shared<ScheduleNode>(shared_from_this(), target_block);
    // Has ScheduledChange
    if (scheduled_after != INACTIVE) {
      if (finalized && scheduled_after <= target_block.number) {
        node->actual_authorities = scheduled_authorities;
      } else {
        node->actual_authorities = actual_authorities;
        node->enabled = enabled;
        node->scheduled_after = scheduled_after;
        node->scheduled_authorities = scheduled_authorities;
      }
    }
    // Has ForcedChange
    else if (forced_for != INACTIVE) {
      if (forced_for <= target_block.number) {
        node->actual_authorities = forced_authorities;
      } else {
        node->actual_authorities = actual_authorities;
        node->enabled = enabled;
        node->forced_for = forced_for;
        node->forced_authorities = forced_authorities;
      }
    }
    // Has planned pause
    else if (pause_after != INACTIVE) {
      node->actual_authorities = actual_authorities;
      if (finalized && pause_after <= target_block.number) {
        node->enabled = false;
      } else {
        node->enabled = enabled;
        node->pause_after = pause_after;
      }
    }
    // Has planned resume
    else if (resume_for != INACTIVE) {
      node->actual_authorities = actual_authorities;
      if (resume_for <= target_block.number) {
        node->enabled = true;
      } else {
        node->enabled = enabled;
        node->resume_for = resume_for;
      }
    }
    // Nothing else
    else {
      node->actual_authorities = actual_authorities;
      node->enabled = enabled;
    }
    return node;
  }
}  // namespace kagome::authority

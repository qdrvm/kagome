/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/schedule_node.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"

namespace kagome::authority {

  outcome::result<void> ScheduleNode::ensureReadyToSchedule() const {
    if (scheduled_after) {
      return AuthorityUpdateObserverError::NO_SCHEDULED_CHANGE_APPLIED_YET;
    }
    if (forced_for) {
      return AuthorityUpdateObserverError::NO_FORCED_CHANGE_APPLIED_YET;
    }
    if (pause_after) {
      return AuthorityUpdateObserverError::NO_PAUSE_APPLIED_YET;
    }
    if (resume_for) {
      return AuthorityUpdateObserverError::NO_RESUME_APPLIED_YET;
    }
    return outcome::success();
  }

  std::shared_ptr<ScheduleNode> ScheduleNode::makeDescendant(
      const primitives::BlockInfo &block) {
    // Has ScheduledChange
    if (scheduled_after) {
      auto node = std::make_shared<ScheduleNode>(shared_from_this(), block);
      node->actual_authorities = actual_authorities;
      node->enabled = enabled;
      node->scheduled_authorities = scheduled_authorities;
      node->scheduled_after = scheduled_after;
      return node;
    }
    // Has ForcedChange
    if (forced_for) {
      auto node = std::make_shared<ScheduleNode>(shared_from_this(), block);
      node->enabled = enabled;
      if (forced_for > block.block_number) {
        node->actual_authorities = scheduled_authorities;
        node->forced_authorities = forced_authorities;
      } else {
        node->actual_authorities = scheduled_authorities;
      }
      return node;
    }
    // Has planned pause
    if (pause_after) {
      auto node = std::make_shared<ScheduleNode>(shared_from_this(), block);
      node->actual_authorities = actual_authorities;
      node->enabled = enabled;
      node->pause_after = pause_after;
      return node;
    }
    // Has planned resume
    if (resume_for) {
      auto node = std::make_shared<ScheduleNode>(shared_from_this(), block);
      node->actual_authorities = actual_authorities;
      if (resume_for <= block.block_number) {
        node->enabled = true;
      }
      return node;
    }
    return shared_from_this();
  }

}  // namespace kagome::authority

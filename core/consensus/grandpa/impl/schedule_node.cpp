/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "schedule_node.hpp"
#include "consensus/grandpa/grandpa_digest_observer_error.hpp"

namespace kagome::consensus::grandpa {

  ScheduleNode::ScheduleNode(
      const std::shared_ptr<const ScheduleNode> &ancestor,
      primitives::BlockInfo block)
      : block(block), parent(ancestor) {
    BOOST_ASSERT(ancestor != nullptr);
  }

  std::shared_ptr<ScheduleNode> ScheduleNode::createAsRoot(
      std::shared_ptr<const primitives::AuthoritySet> current_authorities,
      primitives::BlockInfo block) {
    auto fake_parent = std::make_shared<ScheduleNode>(ScheduleNode());
    auto node = std::make_shared<ScheduleNode>(fake_parent, block);
    node->authorities = current_authorities;
    return node;
  }

  void ScheduleNode::adjust(IsBlockFinalized finalized) {
    if (auto scheduled_change = boost::get<ScheduledChange>(&action);
        finalized && scheduled_change != nullptr) {
      if (scheduled_change->applied_block <= block.number) {
        authorities = std::move(scheduled_change->new_authorities);
        action = NoAction{};
        forced_digests = {};
      }
    } else if (auto pause = boost::get<Pause>(&action);
               finalized && pause != nullptr) {
      if (pause->applied_block <= block.number) {
        enabled = false;
        action = NoAction{};
      }
    } else if (auto forced_change = boost::get<ForcedChange>(&action);
               forced_change != nullptr) {
      if (forced_change->delay_start + forced_change->delay_length
          <= block.number) {
        authorities = std::move(forced_change->new_authorities);
        action = NoAction{};
      }
    } else if (auto resume = boost::get<Resume>(&action); resume != nullptr) {
      if (resume->applied_block <= block.number) {
        enabled = true;
        action = NoAction{};
      }
    }
  }

  std::shared_ptr<ScheduleNode> ScheduleNode::makeDescendant(
      const primitives::BlockInfo &target_block,
      IsBlockFinalized finalized) const {
    auto node =
        std::make_shared<ScheduleNode>(shared_from_this(), target_block);
    node->authorities = authorities;
    node->enabled = enabled;
    node->action = action;
    node->forced_digests = forced_digests;
    node->adjust(finalized);
    return node;
  }
}  // namespace kagome::consensus::grandpa

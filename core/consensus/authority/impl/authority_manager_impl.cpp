/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"

namespace kagome::authority {

  outcome::result<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block) {
    auto node = getAppropriateAncestor(block);

    auto adjusted_node = node->makeDescendant(block);

    if (adjusted_node->enabled) {
      // Original authorities
      return adjusted_node->actual_authorities;
    } else {
      // Zero-weighted authorities
      auto authorities = std::make_shared<primitives::AuthorityList>(
          *adjusted_node->actual_authorities);
      std::for_each(authorities->begin(),
                    authorities->end(),
                    [](auto &authority) { authority.weight = 0; });
      return std::move(authorities);
    }
  }

  outcome::result<void> AuthorityManagerImpl::onScheduledChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    // Schedule change
    new_node->scheduled_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_node->scheduled_after = activate_at;

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          isDirectAncestry(block, descendant->block) ? new_node : node;

      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::inactive;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    // Force changes
    if (new_node->block.block_number >= activate_at) {
      new_node->actual_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
    } else {
      new_node->forced_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_node->forced_for = activate_at;
    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          isDirectAncestry(block, descendant->block) ? new_node : node;

      // Apply forced changes if dalay will be passed for descendant
      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::inactive;
      }
      if (descendant->block.block_number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::inactive;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onOnDisabled(
      const primitives::BlockInfo &block, uint64_t authority_index) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    // Check if index not out of bound
    if (authority_index >= node->actual_authorities->size()) {
      return AuthorityUpdateObserverError::WRONG_AUTHORITY_INDEX;
    }

    // Make changed authorities
    auto authorities = std::make_shared<primitives::AuthorityList>(
        *new_node->actual_authorities);
    (*authorities)[authority_index].weight = 0;
    node->actual_authorities = std::move(authorities);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      if (isDirectAncestry(block, descendant->block)) {
        // Propogate change to descendants
        if (descendant->actual_authorities == node->actual_authorities) {
          descendant->actual_authorities = new_node->actual_authorities;
        }
        new_node->descendants.emplace_back(std::move(descendant));
      } else {
        node->descendants.emplace_back(std::move(descendant));
      }
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onPause(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->pause_after = activate_at;

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          isDirectAncestry(block, descendant->block) ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onResume(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          isDirectAncestry(block, descendant->block) ? new_node : node;

      // Apply resume if delay will be passed for descendant
      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::inactive;
      }
      if (descendant->block.block_number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::inactive;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::ConsensusEngineId &engine_id,
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    if (std::find(known_engines.begin(), known_engines.end(), engine_id)
        == known_engines.end()) {
      return AuthorityManagerError::UNKNOWN_ENGINE_ID;
    }

    return visit_in_place(
        message.payload,
        [this, &block](
            const primitives::ScheduledChange &msg) -> outcome::result<void> {
          return onScheduledChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::ForcedChange &msg) {
          return onForcedChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::OnDisabled &msg) {
          return onOnDisabled(block, msg.authority_index);
        },
        [this, &block](const primitives::Pause &msg) {
          return onPause(block, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::Resume &msg) {
          return onResume(block, block.block_number + msg.subchain_lenght);
        },
        [](auto &) {
          return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
        });
  }

  void AuthorityManagerImpl::onFinalize(const primitives::BlockInfo &block) {
    auto node = getAppropriateAncestor(block);

    // Create new node
    auto new_node = std::make_shared<ScheduleNode>(node, block);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          isDirectAncestry(block, descendant->block) ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    root_ = new_node;
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) {
    std::shared_ptr<ScheduleNode> ancestor = root_;
    for (;;) {
      if (ancestor->block == block) {
        return ancestor;
      }
      bool goto_next_generation = false;
      for (const auto &node : ancestor->descendants) {
        if (isDirectAncestry(node->block, block)) {
          ancestor = node;
          goto_next_generation = true;
          break;
        }
      }
      if (goto_next_generation) continue;
      return ancestor;
    }
  }

  bool AuthorityManagerImpl::isDirectAncestry(
      const primitives::BlockInfo &ancestor,
      const primitives::BlockInfo &descendant) {
#warning  // TODO(xDimon): Needs to be implemented
    return false;
  }
}  // namespace kagome::authority

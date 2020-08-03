/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "scale/scale.hpp"

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<primitives::BabeConfiguration> genesis_configuration,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::BufferStorage> storage)
      : log_{common::createLogger("AuthMngr")},
        app_state_manager_(std::move(app_state_manager)),
        genesis_configuration_(std::move(genesis_configuration)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(genesis_configuration_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  void AuthorityManagerImpl::prepare() {
    auto encoded_root_res = storage_->get(SCHEDULER_TREE);
    if (!encoded_root_res.has_value()) {
      // Get initial authorities from genesis
      root_ = ScheduleNode::createAsRoot({});
      root_->actual_authorities = std::make_shared<primitives::AuthorityList>(
          genesis_configuration_->genesis_authorities);
      return;
    }

    auto root_res =
        scale::decode<std::shared_ptr<ScheduleNode>>(encoded_root_res.value());
    if (!root_res.has_value()) {
      log_->critical("Can't decode stored state");
      app_state_manager_->shutdown();
      return;
    }

    root_ = std::move(root_res.value());
  }

  void AuthorityManagerImpl::start() {}

  void AuthorityManagerImpl::stop() {
    if (!root_) return;
    auto data_res = scale::encode(root_);
    if (!data_res.has_value()) {
      log_->critical("Can't encode state to store");
      return;
    }
    auto save_res =
        storage_->put(SCHEDULER_TREE, common::Buffer(data_res.value()));
    if (save_res.has_value()) {
      log_->critical("Can't store current state");
      return;
    }
  }

  outcome::result<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALISED;
    }

    auto adjusted_node =
        (node->block == block) ? node : node->makeDescendant(block);

    if (adjusted_node->enabled) {
      // Original authorities
      return adjusted_node->actual_authorities;
    }

    // Zero-weighted authorities
    auto authorities = std::make_shared<primitives::AuthorityList>(
        *adjusted_node->actual_authorities);
    std::for_each(authorities->begin(),
                  authorities->end(),
                  [](auto &authority) { authority.weight = 0; });
    return std::move(authorities);
  }

  outcome::result<void> AuthorityManagerImpl::applyScheduledChange(
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
          directChainExists(block, descendant->block) ? new_node : node;

      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
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
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply forced changes if dalay will be passed for descendant
      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.block_number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyOnDisabled(
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
    new_node->actual_authorities = std::move(authorities);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      if (directChainExists(block, descendant->block)) {
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

  outcome::result<void> AuthorityManagerImpl::applyPause(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->pause_after = activate_at;

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyResume(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply resume if delay will be passed for descendant
      if (descendant->block.block_number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.block_number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
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
          return applyScheduledChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::ForcedChange &msg) {
          return applyForcedChange(
              block, msg.authorities, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::OnDisabled &msg) {
          return applyOnDisabled(block, msg.authority_index);
        },
        [this, &block](const primitives::Pause &msg) {
          return applyPause(block, block.block_number + msg.subchain_lenght);
        },
        [this, &block](const primitives::Resume &msg) {
          return applyResume(block, block.block_number + msg.subchain_lenght);
        },
        [](auto &) {
          return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
        });
  }

  outcome::result<void> AuthorityManagerImpl::onFinalize(
      const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return outcome::success();
    }

    if (block.block_number < root_->block.block_number) {
      return AuthorityManagerError::WRONG_FINALISATION_ORDER;
    }

    auto node = getAppropriateAncestor(block);

    // Create new node
    auto new_node = node->makeDescendant(block, true);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      if (directChainExists(block, descendant->block)) {
        new_node->descendants.emplace_back(std::move(descendant));
      }
    }

    root_ = std::move(new_node);

    OUTCOME_TRY(encoded_state, scale::encode(root_));
    OUTCOME_TRY(storage_->put(SCHEDULER_TREE,
                              common::Buffer(std::move(encoded_state))));

    return outcome::success();
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) {
    BOOST_ASSERT(root_ != nullptr);
    std::shared_ptr<ScheduleNode> ancestor;
    // Target block is not descendant of the current root
    if (root_->block.block_number > block.block_number
        || (root_->block != block
            && not directChainExists(root_->block, block))) {
      return ancestor;
    }
    ancestor = root_;
    while (ancestor->block != block) {
      bool goto_next_generation = false;
      for (const auto &node : ancestor->descendants) {
        if (node->block == block || directChainExists(node->block, block)) {
          ancestor = node;
          goto_next_generation = true;
          break;
        }
      }
      if (not goto_next_generation) {
        break;
      }
    }
    return ancestor;
  }

  bool AuthorityManagerImpl::directChainExists(
      const primitives::BlockInfo &ancestor,
      const primitives::BlockInfo &descendant) {
    // Any block is descendant of genesis
    if (ancestor.block_number <= 1
        && ancestor.block_number < descendant.block_number) {
      return true;
    }
    auto result = ancestor.block_number < descendant.block_number
                  && block_tree_->hasDirectChain(ancestor.block_hash,
                                                 descendant.block_hash);
    return result;
  }
}  // namespace kagome::authority

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/authority/impl/authority_manager_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/visitor.hpp"
#include "consensus/authority/authority_manager_error.hpp"
#include "consensus/authority/authority_update_observer_error.hpp"
#include "consensus/authority/impl/schedule_node.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::BufferStorage> storage)
      : log_{log::createLogger("AuthorityManager", "authority")},
        app_state_manager_(std::move(app_state_manager)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool AuthorityManagerImpl::prepare() {
    auto encoded_root_res = storage_->get(storage::kSchedulerTreeLookupKey);
    if (not encoded_root_res.has_value()) {
      log_->critical("Can't restore authority manager state");
      return false;
    }

    auto root_res =
        scale::decode<std::shared_ptr<ScheduleNode>>(encoded_root_res.value());
    if (!root_res.has_value()) {
      log_->critical("Can't decode stored state");
      return false;
    }
    auto &root = root_res.value();

    root_ = std::move(root);

    return true;
  }

  bool AuthorityManagerImpl::start() {
    return true;
  }

  void AuthorityManagerImpl::stop() {
    if (!root_) return;
    std::ignore = save();
  }

  outcome::result<void> AuthorityManagerImpl::save() {
    BOOST_ASSERT(root_ != nullptr);

    auto data_res = scale::encode(root_);
    if (!data_res.has_value()) {
      log_->critical("Can't encode state to store");
      return AuthorityManagerError::CAN_NOT_SAVE_STATE;
    }

    auto save_res = storage_->put(storage::kSchedulerTreeLookupKey,
                                  common::Buffer(data_res.value()));
    if (!save_res.has_value()) {
      log_->critical("Can't store current state");
      return AuthorityManagerError::CAN_NOT_SAVE_STATE;
    }

    return outcome::success();
  }

  primitives::BlockInfo AuthorityManagerImpl::base() const {
    if (not root_) {
      log_->critical("Authority manager has null root");
      std::terminate();
    }
    return root_->block;
  }

  outcome::result<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block,
                                    bool finalized) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALISED;
    }

    auto adjusted_node = node->makeDescendant(block, finalized);

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
    return authorities;
  }

  outcome::result<void> AuthorityManagerImpl::applyScheduledChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    SL_DEBUG(log_,
             "Applying scheduled change for block {} to activate at block {}",
             block,
             activate_at);
    auto node = getAppropriateAncestor(block);
    SL_DEBUG(
        log_,
        "Oldest scheduled change before block {} is at block {} with set id {}",
        block,
        node->block,
        node->actual_authorities->id);
    auto last_finalized = block_tree_->getLastFinalized();
    bool block_in_finalized_chain =
        block_tree_->hasDirectChain(block.hash, last_finalized.hash);
    SL_DEBUG(log_,
             "Last finalized is {}, is on the same chain as target block? {}",
             last_finalized,
             block_in_finalized_chain);

    auto new_node = node->makeDescendant(
        block,
        block.number <= last_finalized.number && block_in_finalized_chain);
    SL_DEBUG(log_,
             "Make a schedule node for block {}, with actual set id {}",
             block,
             new_node->actual_authorities->id);

    auto res = new_node->ensureReadyToSchedule();
    if (!res) {
      SL_DEBUG(
          log_, "Node is not ready to be scheduled: {}", res.error().message());
      return res.as_failure();
    }

    auto new_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_authorities->id = new_node->actual_authorities->id + 1;

    // Schedule change
    new_node->scheduled_authorities = std::move(new_authorities);
    new_node->scheduled_after = activate_at;

    SL_VERBOSE(log_,
               "Change is scheduled after block #{} (set id={})",
               new_node->scheduled_after,
               new_node->scheduled_authorities->id);
    for (auto &authority : *new_node->scheduled_authorities) {
      SL_VERBOSE(log_,
                 "New authority id={}, weight={}",
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    std::ignore = save();

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyForcedChange(
      const primitives::BlockInfo &block,
      const primitives::AuthorityList &authorities,
      primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    auto new_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_authorities->id = new_node->actual_authorities->id + 1;

    // Force changes
    if (new_node->block.number >= activate_at) {
      new_node->actual_authorities = std::move(new_authorities);
    } else {
      new_node->forced_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_node->forced_for = activate_at;
    }

    SL_VERBOSE(
        log_, "Change is forced on block #{}", new_node->scheduled_after);
    for (auto &authority : *new_node->forced_authorities) {
      SL_VERBOSE(log_,
                 "New authority id={}, weight={}",
                 authority.id.id,
                 authority.weight);
    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply forced changes if dalay will be passed for descendant
      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    std::ignore = save();

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

    SL_VERBOSE(log_,
               "Authotity id={} is disabled on block #{}",
               (*authorities)[authority_index].id.id,
               new_node->block.number);

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

    std::ignore = save();

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyPause(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->pause_after = activate_at;

    SL_VERBOSE(log_, "Scheduled pause after block #{}", new_node->block.number);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;
      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    std::ignore = save();

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::applyResume(
      const primitives::BlockInfo &block, primitives::BlockNumber activate_at) {
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    new_node->resume_for = activate_at;

    SL_VERBOSE(log_, "Scheduled resume on block #{}", new_node->block.number);

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      auto &ancestor =
          directChainExists(block, descendant->block) ? new_node : node;

      // Apply resume if delay will be passed for descendant
      if (descendant->block.number >= ancestor->forced_for) {
        descendant->actual_authorities = ancestor->forced_authorities;
        descendant->forced_authorities.reset();
        descendant->forced_for = ScheduleNode::INACTIVE;
      }
      if (descendant->block.number >= ancestor->resume_for) {
        descendant->enabled = true;
        descendant->resume_for = ScheduleNode::INACTIVE;
      }

      ancestor->descendants.emplace_back(std::move(descendant));
    }
    node->descendants.emplace_back(std::move(new_node));

    std::ignore = save();

    return outcome::success();
  }

  outcome::result<void> AuthorityManagerImpl::onConsensus(
      const primitives::ConsensusEngineId &engine_id,
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    OUTCOME_TRY(message.decode());
    if (engine_id == primitives::kBabeEngineId) {
      // TODO(xDimon): Perhaps it needs to be refactored.
      //  It is better handle babe digests here
      //  Issue: https://github.com/soramitsu/kagome/issues/740
      return visit_in_place(
          message.asBabeDigest(),
          [](const primitives::NextEpochData &msg) -> outcome::result<void> {
            return outcome::success();
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
          },
          [](const primitives::NextConfigData &msg) {
            return outcome::success();
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else if (engine_id == primitives::kGrandpaEngineId) {
      return visit_in_place(
          message.asGrandpaDigest(),
          [this, &block](
              const primitives::ScheduledChange &msg) -> outcome::result<void> {
            return applyScheduledChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::ForcedChange &msg) {
            return applyForcedChange(
                block, msg.authorities, block.number + msg.subchain_length);
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
          },
          [this, &block](const primitives::Pause &msg) {
            return applyPause(block, block.number + msg.subchain_length);
          },
          [this, &block](const primitives::Resume &msg) {
            return applyResume(block, block.number + msg.subchain_length);
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else {
      return AuthorityManagerError::UNKNOWN_ENGINE_ID;
    }
  }

  outcome::result<void> AuthorityManagerImpl::prune(
      const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return outcome::success();
    }

    if (block.number < root_->block.number) {
      return outcome::success();
    }

    auto node = getAppropriateAncestor(block);

    if (node->block == block) {
      // Rebase
      root_ = std::move(node);

    } else {
      // Reorganize ancestry
      auto new_node = node->makeDescendant(block, true);
      for (auto &descendant : std::move(node->descendants)) {
        if (directChainExists(block, descendant->block)) {
          new_node->descendants.emplace_back(std::move(descendant));
        }
      }

      root_ = std::move(new_node);
    }

    SL_VERBOSE(log_, "Prune authority manager upto block #{}", block.number);

    OUTCOME_TRY(save());

    return outcome::success();
  }

  std::shared_ptr<ScheduleNode> AuthorityManagerImpl::getAppropriateAncestor(
      const primitives::BlockInfo &block) {
    BOOST_ASSERT(root_ != nullptr);
    std::shared_ptr<ScheduleNode> ancestor;
    // Target block is not descendant of the current root
    if (root_->block.number > block.number
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
    if (ancestor.number <= 1 && ancestor.number < descendant.number) {
      return true;
    }
    auto result =
        ancestor.number < descendant.number
        && block_tree_->hasDirectChain(ancestor.hash, descendant.hash);
    return result;
  }
}  // namespace kagome::authority

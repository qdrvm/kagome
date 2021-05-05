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
#include "primitives/babe_configuration.hpp"
#include "scale/scale.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::authority {

  AuthorityManagerImpl::AuthorityManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::BufferStorage> storage)
      : log_{log::createLogger("AuthorityManager", "authority")},
        app_state_manager_(std::move(app_state_manager)),
        grandpa_api_(std::move(grandpa_api)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)) {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);

    app_state_manager_->takeControl(*this);
  }

  bool AuthorityManagerImpl::prepare() {
    auto encoded_root_res = storage_->get(SCHEDULER_TREE);
    if (!encoded_root_res.has_value()) {
      // Get initial authorities from genesis
      auto hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
      if (not hash_res.has_value()) {
        log_->critical("Can't decode genesis block hash");
        return false;
      }
      primitives::BlockHash hash;
      std::copy(hash_res.value().begin(), hash_res.value().end(), hash.begin());

      auto authorities_res = grandpa_api_->authorities(
          primitives::BlockId(primitives::BlockNumber{0}));
      auto &authorities = authorities_res.value();

      root_ = ScheduleNode::createAsRoot({0, hash});
      root_->actual_authorities =
          std::make_shared<primitives::AuthorityList>(std::move(authorities));

      std::ignore = save();
      return true;
    }

    auto root_res =
        scale::decode<std::shared_ptr<ScheduleNode>>(encoded_root_res.value());
    if (!root_res.has_value()) {
      log_->critical("Can't decode stored state");
      return false;
    }
    auto &root = root_res.value();

    root_ = std::move(root);

    if (root_->block.number == 0) {
      auto hash_res = storage_->get(storage::kGenesisBlockHashLookupKey);
      if (not hash_res.has_value()) {
        log_->critical("Can't decode genesis block hash");
        return false;
      }

      primitives::BlockHash &hash =
          const_cast<primitives::BlockHash &>(root_->block.hash);
      std::copy(hash_res.value().begin(), hash_res.value().end(), hash.begin());

      auto authorities_res = grandpa_api_->authorities(
          primitives::BlockId(primitives::BlockNumber{0}));
      auto &authorities = authorities_res.value();

      root_->actual_authorities =
          std::make_shared<primitives::AuthorityList>(std::move(authorities));

      stop();
    }

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

    auto save_res =
        storage_->put(SCHEDULER_TREE, common::Buffer(data_res.value()));
    if (!save_res.has_value()) {
      log_->critical("Can't store current state");
      return AuthorityManagerError::CAN_NOT_SAVE_STATE;
    }

    return outcome::success();
  }

  outcome::result<std::shared_ptr<const primitives::AuthorityList>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &block, bool finalized) {
    auto node = getAppropriateAncestor(block);

    if (not node) {
      return AuthorityManagerError::ORPHAN_BLOCK_OR_ALREADY_FINALISED;
    }

//    auto adjusted_node =
//        (node->block == block) ? node : node->makeDescendant(block);

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
    auto node = getAppropriateAncestor(block);

    auto new_node = node->makeDescendant(block);

    OUTCOME_TRY(new_node->ensureReadyToSchedule());

    // Schedule change
    new_node->scheduled_authorities =
        std::make_shared<primitives::AuthorityList>(authorities);
    new_node->scheduled_after = activate_at;

    SL_VERBOSE(
        log_, "Change is scheduled after block #{}", new_node->scheduled_after);
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

    // Force changes
    if (new_node->block.number >= activate_at) {
      new_node->actual_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
    } else {
      new_node->forced_authorities =
          std::make_shared<primitives::AuthorityList>(authorities);
      new_node->forced_for = activate_at;
    }

    SL_VERBOSE(
        log_, "Change is forced on block #{}", new_node->scheduled_after);
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
      return visit_in_place(
          message.asBabeDigest(),
          [this, &block](
              const primitives::NextEpochData &msg) -> outcome::result<void> {
            (void)this;
            (void)block;
            return outcome::success();
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
          },
          [this, &block](const primitives::NextConfigData &msg) {
            (void)this;
            (void)block;
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
                block, msg.authorities, block.number + msg.subchain_lenght);
          },
          [this, &block](const primitives::ForcedChange &msg) {
            return applyForcedChange(
                block, msg.authorities, block.number + msg.subchain_lenght);
          },
          [](const primitives::OnDisabled &msg) {
            // Note: This event type wount be used anymore and must be ignored
            return outcome::success();
          },
          [this, &block](const primitives::Pause &msg) {
            return applyPause(block, block.number + msg.subchain_lenght);
          },
          [this, &block](const primitives::Resume &msg) {
            return applyResume(block, block.number + msg.subchain_lenght);
          },
          [](auto &) {
            return AuthorityUpdateObserverError::UNSUPPORTED_MESSAGE_TYPE;
          });
    } else {
      return AuthorityManagerError::UNKNOWN_ENGINE_ID;
    }
  }

  outcome::result<void> AuthorityManagerImpl::onFinalize(
      const primitives::BlockInfo &block) {
    if (block == root_->block) {
      return outcome::success();
    }

    if (block.number < root_->block.number) {
      return AuthorityManagerError::WRONG_FINALISATION_ORDER;
    }

    auto node = getAppropriateAncestor(block);

    // Create new node
    auto new_node = node->makeDescendant(block, true);

//    // Update actual authorities
//    auto authorities_res = grandpa_api_->authorities(block.hash);
//    if (authorities_res.has_value()) {
//      auto authorities = std::make_shared<primitives::AuthorityList>(
//          std::move(authorities_res.value()));
//      new_node->forced_authorities = std::move(authorities);
//      new_node->forced_for = block.number + 1;
//    }

    // Reorganize ancestry
    for (auto &descendant : std::move(node->descendants)) {
      if (directChainExists(block, descendant->block)) {
//        descendant->actual_authorities = new_node->actual_authorities;
        new_node->descendants.emplace_back(std::move(descendant));
      }
    }

    root_ = std::move(new_node);

    SL_VERBOSE(
        log_, "Reorganize authority at filalizaion of block #{}", block.number);

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

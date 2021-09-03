/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/common/storage_code_provider.hpp"

namespace kagome::runtime {

  RuntimeUpgradeTrackerImpl::RuntimeUpgradeTrackerImpl(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : header_repo_{std::move(header_repo)},
        logger_{log::createLogger("StorageCodeProvider", "runtime")} {
    BOOST_ASSERT(header_repo_);
  }

  outcome::result<storage::trie::RootHash>
  RuntimeUpgradeTrackerImpl::getLastCodeUpdateState(
      const primitives::BlockInfo &block) const {
    // if the block tree is not yet initialized, means we can only access the
    // genesis block
    if (block_tree_ == nullptr) {
      OUTCOME_TRY(genesis, header_repo_->getBlockHeader(0));
      logger_->debug("Pick runtime state at genesis for block #{} hash {}",
                     block.number,
                     block.hash.toHex());
      return genesis.state_root;
    }

    // if there are no known blocks with runtime upgrades, we just fall back to
    // returning the state of the current block
    if (runtime_upgrade_parents_.empty()) {
      // even if it doesn't actually upgrade runtime, still a solid source of
      // runtime code
      OUTCOME_TRY(header, header_repo_->getBlockHeader(block.hash));
      if (block.number == 0) {
        runtime_upgrade_parents_.emplace_back(0, block.hash);
      } else {
        runtime_upgrade_parents_.emplace_back(block.number - 1,
                                              header.parent_hash);
      }
      logger_->debug(
          "Pick runtime state at block #{} hash {} for the same block",
          block.number,
          block.hash.toHex());
      return header.state_root;
    }

    KAGOME_PROFILE_START(blocks_with_runtime_upgrade_search)
    auto block_number = block.number;
    auto latest_state_update_it =
        std::upper_bound(runtime_upgrade_parents_.begin(),
                         runtime_upgrade_parents_.end(),
                         block_number,
                         [](auto block_number, auto const &block_info) {
                           return block_number < block_info.number;
                         });
    KAGOME_PROFILE_END(blocks_with_runtime_upgrade_search)
    if (latest_state_update_it == runtime_upgrade_parents_.begin()) {
      // if we have no info on updates before this block, we just return its
      // state
      OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
      logger_->debug(
          "Pick runtime state at block #{} hash {} for the same block",
          block.number,
          block.hash.toHex());
      return block_header.state_root;
    }

    latest_state_update_it--;
    // we are now at the last element in block_with_runtime_upgrade which is
    // less or equal to our \arg block number
    // we may have several entries with the same block number, we have to pick
    // one which is the predecessor of our block
    KAGOME_PROFILE_START(search_for_proper_fork);
    for (;
         std::next(latest_state_update_it) != runtime_upgrade_parents_.begin();
         latest_state_update_it--) {
      bool state_in_the_same_chain = false;

      // if the found state is finalized, it is guaranteed to not belong to a
      // different fork
      if (block_tree_->getLastFinalized().number
          >= latest_state_update_it->number) {
        state_in_the_same_chain = true;
      } else {
        // a non-finalized state may belong to a different fork, need to check
        // explicitly (can be expensive if blocks are far apart)
        KAGOME_PROFILE_START(has_direct_chain);
        bool has_direct_chain = block_tree_->hasDirectChain(
            latest_state_update_it->hash, block.hash);
        KAGOME_PROFILE_END(has_direct_chain);
        state_in_the_same_chain = has_direct_chain;
      }

      if (state_in_the_same_chain) {
        if (latest_state_update_it->number == 0) {
          OUTCOME_TRY(genesis, header_repo_->getBlockHeader(0));
          SL_TRACE_FUNC_CALL(
              logger_, genesis.state_root, block.hash, block.number);
          logger_->debug(
              "Pick runtime state at genesis for block #{} hash {}",
              block.number,
              block.hash.toHex());
          return genesis.state_root;
        }

        // found the predecessor with the latest runtime upgrade
        auto children_res = block_tree_->getChildren(latest_state_update_it->hash);
        if(!children_res) {
          return children_res.error();
        }
        auto children = std::move(children_res.value());
        // temporary; need to find a way to tackle forks here
        // the runtime upgrades are reported for the state of the parent of the
        // block with the runtime upgrade, so we need to fetch a child block
        BOOST_ASSERT(children.size() == 1);
        OUTCOME_TRY(target_header, header_repo_->getBlockHeader(children[0]));
        BOOST_ASSERT(block_tree_->getLastFinalized().number
                         >= target_header.number
                     or block_tree_->hasDirectChain(children[0], block.hash));

        SL_TRACE_FUNC_CALL(
            logger_, target_header.state_root, block.hash, block.number);
        logger_->debug(
            "Pick runtime state at block #{} hash {} for block #{} hash {}",
            target_header.number,
            children[0],
            block.number,
            block.hash.toHex());
        return target_header.state_root;
      }
    }
    KAGOME_PROFILE_END(search_for_proper_fork);
    // if this is an orphan block for some reason, just return its state_root
    // (there is no other choice)
    OUTCOME_TRY(block_header, header_repo_->getBlockHeader(block.hash));
    logger_->warn("Block #{} hash {}, a child of block with hash {} is orphan",
                  block.number,
                  block.hash.toHex(),
                  block_header.parent_hash.toHex());
    return block_header.state_root;
  }

  void RuntimeUpgradeTrackerImpl::subscribeToBlockchainEvents(
      std::shared_ptr<primitives::events::StorageSubscriptionEngine>
          storage_sub_engine,
      std::shared_ptr<const blockchain::BlockTree> block_tree) {
    storage_subscription_ =
        std::make_shared<primitives::events::StorageEventSubscriber>(
            storage_sub_engine);
    block_tree_ = std::move(block_tree);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_subscription_ != nullptr);

    auto storage_subscription_set_id =
        storage_subscription_->generateSubscriptionSetId();
    storage_subscription_->subscribe(storage_subscription_set_id,
                                     kRuntimeCodeKey);
    // might need to capture a weak pointer to this which might be painful in
    // constructor
    storage_subscription_->setCallback([this](auto set_id,
                                              auto &receiver,
                                              auto &type,
                                              auto &new_value,
                                              auto &block_hash) {
      SL_DEBUG(logger_, "Runtime upgrade at block {}", block_hash.toHex());
      auto number = header_repo_->getNumberByHash(block_hash).value();
      auto it = std::upper_bound(runtime_upgrade_parents_.begin(),
                                 runtime_upgrade_parents_.end(),
                                 number,
                                 [](auto &number, auto &block_id) {
                                   return number < block_id.number;
                                 });
      runtime_upgrade_parents_.emplace(it, number, block_hash);
    });
  }

}  // namespace kagome::runtime

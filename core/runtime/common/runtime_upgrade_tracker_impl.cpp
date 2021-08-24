/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_upgrade_tracker_impl.hpp"

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "runtime/common/storage_code_provider.hpp"

namespace kagome::runtime {

  RuntimeUpgradeTrackerImpl::RuntimeUpgradeTrackerImpl(
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      const application::CodeSubstitutes &code_substitutes)
      : header_repo_{std::move(header_repo)},
        code_substitutes_{code_substitutes},
        logger_{log::createLogger("StorageCodeProvider", "runtime")} {
    BOOST_ASSERT(header_repo_);
  }

  outcome::result<std::tuple<primitives::BlockId, bool>>
  RuntimeUpgradeTrackerImpl::getRuntimeChangeBlock(
      const primitives::BlockInfo &block) const {
    if (block.number == 0 && blocks_with_runtime_upgrade_.empty()) {
      blocks_with_runtime_upgrade_.push_back(block);
    }
    // TODO(Harrm): check if this can lead to incorrect behaviour
    // if the block tree is not yet initialized, means we can only access the
    // genesis block
    if (block_tree_ == nullptr) {
      logger_->debug("Pick runtime state at genesis for block #{} hash {}",
                     block.number,
                     block.hash.toHex());
      return {0, true};
    }

    // if there are no known blocks with runtime upgrades, we just fall back to
    // returning the state of the current block
    if (blocks_with_runtime_upgrade_.empty()) {
      logger_->debug(
          "Pick runtime state at block #{} hash {} for the same block",
          block.number,
          block.hash.toHex());
      return {block.hash, false};
    }

    if (auto it = code_substitutes_.find(block.hash);
        it != code_substitutes_.end()) {
      blocks_with_runtime_upgrade_.push_back(block);
      return {block.hash, true};
    }

    auto block_number = block.number;
    auto latest_state_update_it =
        std::upper_bound(blocks_with_runtime_upgrade_.begin(),
                         blocks_with_runtime_upgrade_.end(),
                         block_number,
                         [](auto block_number, auto const &block_info) {
                           return block_number < block_info.number;
                         });
    if (latest_state_update_it == blocks_with_runtime_upgrade_.begin()) {
      // if we have no info on updates before this block, we just return its
      // state
      // TODO(Harrm): fetch code updates from block tree on initialization
      logger_->debug(
          "Pick runtime state at block #{} hash {} for the same block",
          block.number,
          block.hash.toHex());
      return {block.hash, false};
    }
    latest_state_update_it--;

    // we are now at the last element in block_with_runtime_upgrade which is
    // less or equal to our \arg block number
    // we may have several entries with the same block number, we have to pick
    // one which is the predecessor of our block
    for (; std::next(latest_state_update_it)
           != blocks_with_runtime_upgrade_.begin();
         latest_state_update_it--) {
      if (block_tree_->hasDirectChain(latest_state_update_it->hash,
                                      block.hash)) {
        logger_->debug(
            "Pick runtime state at block #{} hash {} for block #{} hash {}",
            latest_state_update_it->number,
            latest_state_update_it->hash,
            block.number,
            block.hash.toHex());
        return {block.hash, true};
      }
    }

    // if this is an orphan block for some reason, just return its state_root
    // (there is no other choice)
    logger_->warn(
        "Block #{} hash {} is orphan", block.number, block.hash.toHex());
    return {block.hash, false};
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
      if (blocks_with_runtime_upgrade_.empty()) {
        BOOST_ASSERT_MSG(number == 0,
                         "First runtime 'update' is its initial insertion from "
                         "genesis data");
        blocks_with_runtime_upgrade_.emplace_back(number, block_hash);
      }
      auto it = std::upper_bound(blocks_with_runtime_upgrade_.begin(),
                                 blocks_with_runtime_upgrade_.end(),
                                 number,
                                 [](auto &number, auto &block_id) {
                                   return number < block_id.number;
                                 });
      BOOST_ASSERT(it != blocks_with_runtime_upgrade_.begin());
      blocks_with_runtime_upgrade_.emplace(it, number, block_hash);
    });
  }

}  // namespace kagome::runtime

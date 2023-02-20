/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consistency_keeper_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/digest_tracker.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::babe {

  ConsistencyKeeperImpl::ConsistencyKeeperImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::DigestTracker> digest_tracker)
      : app_state_manager_(std::move(app_state_manager)),
        storage_(storage->getSpace(storage::Space::kDefault)),
        block_tree_(std::move(block_tree)),
        digest_tracker_(std::move(digest_tracker)),
        logger_{log::createLogger("ConsistencyKeeper", "block_executor")} {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(digest_tracker_ != nullptr);

    app_state_manager_->atPrepare([&] { return prepare(); });
  }

  bool ConsistencyKeeperImpl::prepare() {
    // try to get record
    auto buf_opt_res = storage_->tryGet(storage::kApplyingBlockInfoLookupKey);
    if (buf_opt_res.has_error()) {
      SL_WARN(logger_,
              "Can't check existence of partial applied block",
              buf_opt_res.error());
      return false;
    }

    // check if record exists
    auto &buf_opt = buf_opt_res.value();
    if (not buf_opt.has_value()) {
      return true;
    }

    // decode obtained record
    auto block_res = scale::decode<primitives::BlockInfo>(buf_opt.value());
    if (block_res.has_value()) {
      auto &block = block_res.value();

      SL_WARN(logger_,
              "Found partial applied block {}. Trying to rollback it",
              block);

      rollback(block);
      return true;
    }

    SL_WARN(logger_,
            "Can't check existence of partial applied block",
            block_res.error());
    return false;
  }

  ConsistencyGuard ConsistencyKeeperImpl::start(
      primitives::BlockInfo block) {
    bool val = false;
    BOOST_VERIFY_MSG(in_progress_.compare_exchange_strong(val, true),
                     "Allowed only one block applying in any time");

    // remove record from storage
    auto put_res = storage_->put(storage::kApplyingBlockInfoLookupKey,
                                 common::Buffer(scale::encode(block).value()));

    if (put_res.has_error()) {
      SL_WARN(logger_,
              "Can't store record of partial applied block",
              put_res.error());
    }

    SL_DEBUG(logger_, "Start applying of block {}", block);
    return ConsistencyGuard(*this, block);
  }

  void ConsistencyKeeperImpl::commit(primitives::BlockInfo block) {
    cleanup();
    SL_DEBUG(logger_, "Applying of block {} finished successfully", block);
  }

  void ConsistencyKeeperImpl::rollback(primitives::BlockInfo block) {
    // Cancel tracked block digest
    digest_tracker_->cancel(block);

    // Remove block as leaf of block tree
    auto removal_res = block_tree_->removeLeaf(block.hash);
    if (removal_res.has_error()) {
      SL_WARN(logger_,
              "Rolling back of block {} is failed: {}",
              block,
              removal_res.error());
    }

    cleanup();
    SL_DEBUG(logger_, "Applying of block {} was rolled back", block);
  }

  void ConsistencyKeeperImpl::cleanup() {
    // remove record from storage
    auto removal_res = storage_->remove(storage::kApplyingBlockInfoLookupKey);
    in_progress_ = false;

    if (removal_res.has_error()) {
      SL_WARN(logger_,
              "Can't remove record of partial applied block",
              removal_res.error());
      return;
    }
  }

}  // namespace kagome::consensus::babe

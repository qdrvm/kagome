/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consistency_keeper_impl.hpp"

#include <boost/assert.hpp>
#include <scale/scale.hpp>

#include "application/app_state_manager.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/predefined_keys.hpp"

namespace kagome::consensus::babe {

  ConsistencyKeeperImpl::ConsistencyKeeperImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<authority::AuthorityUpdateObserver>
          authority_update_observer)
      : app_state_manager_(std::move(app_state_manager)),
        storage_(std::move(storage)),
        block_tree_(std::move(block_tree)),
        authority_update_observer_(std::move(authority_update_observer)),
        logger_{log::createLogger("ConsistencyKeeper", "block_executor")} {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(authority_update_observer_ != nullptr);

    app_state_manager_->atPrepare([&] { return prepare(); });
  }

  bool ConsistencyKeeperImpl::prepare() {
    // try to get record
    auto buf_opt_res = storage_->tryLoad(storage::kApplyingBlockInfoLookupKey);
    if (buf_opt_res.has_error()) {
      SL_WARN(logger_,
              "Can't check existence of partial applied block",
              buf_opt_res.error().message());
      return false;
    }

    // check if record exists
    auto buf_opt = buf_opt_res.value();
    if (not buf_opt.has_value()) {
      return true;
    }

    // decode obtained record
    auto block_res = scale::decode<primitives::BlockInfo>(buf_opt.value());
    if (block_res.has_value()) {
      auto &block = block_res.value();

      SL_WARN(logger_,
              "Found partial applied block {}. Trying to rollback him",
              block);

      rollback(block);
      return true;
    }

    SL_WARN(logger_,
            "Can't check existence of partial applied block",
            block_res.error().message());
    return false;
  }

  ConsistencyKeeper::Guard ConsistencyKeeperImpl::start(
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
              put_res.error().message());
    }

    SL_DEBUG(logger_, "Start applying of block {}", block);
    return ConsistencyKeeper::Guard(*this, block);
  }

  void ConsistencyKeeperImpl::commit(primitives::BlockInfo block) {
    cleanup();
    SL_DEBUG(logger_, "Applying of block {} finished successfully", block);
  }

  void ConsistencyKeeperImpl::rollback(primitives::BlockInfo block) {
    // Remove possible authority changes scheduled on block
    authority_update_observer_->cancel(block);

    // Remove block as leaf of block tree
    auto removal_res = block_tree_->removeLeaf(block.hash);
    if (removal_res.has_error()) {
      SL_WARN(logger_,
              "Rolling back of block {} is failed: {}",
              block,
              removal_res.error().message());
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
              removal_res.error().message());
      return;
    }
  }

}  // namespace kagome::consensus::babe

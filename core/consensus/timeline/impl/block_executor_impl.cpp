/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/block_executor_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/main_thread_pool.hpp"
#include "common/worker_thread_pool.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/timeline/impl/block_addition_error.hpp"
#include "consensus/timeline/impl/block_appender_base.hpp"
#include "metrics/histogram_timer.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "transaction_pool/transaction_pool.hpp"
#include "transaction_pool/transaction_pool_error.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace kagome::consensus {

  metrics::HistogramTimer metric_block_execution_time{
      "kagome_block_verification_and_import_time",
      "Time taken to verify and import blocks",
      {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10},
  };

  BlockExecutorImpl::BlockExecutorImpl(
      application::AppStateManager &app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      common::MainThreadPool &main_thread_pool,
      common::WorkerThreadPool &worker_thread_pool,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      std::unique_ptr<BlockAppenderBase> appender)
      : block_tree_{std::move(block_tree)},
        main_pool_handler_{main_thread_pool.handler(app_state_manager)},
        worker_pool_handler_{
            poolHandlerReadyMake(app_state_manager, worker_thread_pool)},
        core_{std::move(core)},
        tx_pool_{std::move(tx_pool)},
        hasher_{std::move(hasher)},
        offchain_worker_api_(std::move(offchain_worker_api)),
        storage_sub_engine_{std::move(storage_sub_engine)},
        chain_subscription_engine_{std::move(chain_sub_engine)},
        appender_{std::move(appender)},
        logger_{log::createLogger("BlockExecutor", "block_executor")},
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(worker_pool_handler_ != nullptr);
    BOOST_ASSERT(core_ != nullptr);
    BOOST_ASSERT(tx_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(offchain_worker_api_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    BOOST_ASSERT(telemetry_ != nullptr);
    BOOST_ASSERT(appender_ != nullptr);
  }

  BlockExecutorImpl::~BlockExecutorImpl() = default;

  void BlockExecutorImpl::applyBlock(
      primitives::Block &&block,
      const std::optional<primitives::Justification> &justification,
      ApplyJustificationCb &&callback) {
    auto block_info = block.header.blockInfo();
    if (not block_tree_->has(block.header.parent_hash)) {
      callback(BlockAdditionError::PARENT_NOT_FOUND);
      return;
    }

    // get current time to measure performance if block execution
    auto start_time = std::chrono::steady_clock::now();

    bool block_was_applied_earlier = false;

    // check if block body already exists. If so, do not apply
    if (auto body_res = block_tree_->getBlockBody(block_info.hash);
        body_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing block: {}", block_info);

      if (auto res =
              block_tree_->addExistingBlock(block_info.hash, block.header);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }
      block_was_applied_earlier = true;
    } else if (body_res.error() != blockchain::BlockTreeError::BODY_NOT_FOUND) {
      callback(body_res.as_failure());
      return;
    }

    if (auto r = appender_->validateHeader(block); r.has_error()) {
      callback(r.as_failure());
      return;
    }

    // Calculate best block before new one will be applied
    auto previous_best_block = block_tree_->bestBlock();

    if (block_was_applied_earlier) {
      applyBlockExecuted(std::move(block),
                         justification,
                         std::move(callback),
                         block_info,
                         start_time,
                         previous_best_block);
      return;
    }
    auto execute = [this,
                    self{shared_from_this()},
                    block{std::move(block)},
                    justification,
                    callback{std::move(callback)},
                    block_info,
                    start_time,
                    previous_best_block]() mutable {
      auto timer = metric_block_execution_time.manual();

      auto parent =
          block_tree_->getBlockHeader(block.header.parent_hash).value();

      SL_DEBUG(logger_,
               "Execute block {}, state {}, a child of block {}, state {}",
               block_info,
               block.header.state_root,
               primitives::BlockInfo(parent.number, block.header.parent_hash),
               parent.state_root);

      // block should be applied without last digest which contains the seal
      auto changes_tracker =
          std::make_shared<storage::changes_trie::StorageChangesTrackerImpl>();

      primitives::BlockReflection block_ref{
          .header = primitives::UnsealedBlockHeaderReflection(block.header),
          .body = block.body,
      };

      if (auto res = core_->execute_block_ref(block_ref, changes_tracker);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }

      auto duration_ms = timer().count();
      SL_DEBUG(logger_, "Core_execute_block: {} ms", duration_ms);

      // add block header if it does not exist
      if (auto res = block_tree_->addBlock(block); res.has_error()) {
        callback(res.as_failure());
        return;
      }

      changes_tracker->onBlockAdded(
          block_info.hash, storage_sub_engine_, chain_subscription_engine_);

      auto executed = [self,
                       block{std::move(block)},
                       justification{std::move(justification)},
                       callback{std::move(callback)},
                       block_info,
                       start_time,
                       previous_best_block]() mutable {
        self->applyBlockExecuted(std::move(block),
                                 justification,
                                 std::move(callback),
                                 block_info,
                                 start_time,
                                 previous_best_block);
      };
      main_pool_handler_->execute(std::move(executed));
    };
    worker_pool_handler_->execute(std::move(execute));
  }

  void BlockExecutorImpl::applyBlockExecuted(
      primitives::Block &&block,
      const std::optional<primitives::Justification> &justification,
      ApplyJustificationCb &&callback,
      const primitives::BlockInfo &block_info,
      clock::SteadyClock::TimePoint start_time,
      const primitives::BlockInfo &previous_best_block) {
    /// TODO(iceseer): in a case we change the authority set, we can get an
    /// error with the following behavior: the finalisation will commit the
    /// authority change and the step of the next block processing will be
    /// failed because of VRF error

    appender_->applyJustifications(
        block_info,
        justification,
        [callback{std::move(callback)},
         wself{weak_from_this()},
         start_time,
         block{std::move(block)},
         previous_best_block_number{previous_best_block.number},
         block_info](auto &&result) mutable {
          auto self = wself.lock();
          if (result.has_error() || !self) {
            /// no processing, just forward call info callback to be able to
            /// release internal resources
            callback(std::move(result));
            return;
          }

          // remove block's extrinsics from tx pool
          for (const auto &extrinsic : block.body) {
            auto extrinsic_hash = self->hasher_->blake2b_256(extrinsic.data);
            SL_DEBUG(self->logger_,
                     "Contains extrinsic with hash: {}",
                     extrinsic_hash);
            auto res = self->tx_pool_->removeOne(extrinsic_hash);
            if (res.has_error()
                && res
                       != outcome::failure(
                           transaction_pool::TransactionPoolError::
                               TX_NOT_FOUND)) {
              callback(res.as_failure());
              return;
            }
          }

          BlockAppenderBase::SlotInfo slot_info;
          if (auto res = self->appender_->getSlotInfo(block.header);
              res.has_error()) {
            callback(res.as_failure());
            return;
          } else {
            slot_info = res.value();
          }

          auto &[slot_start, slot_duration] = slot_info;
          auto lag = std::chrono::system_clock::now() - slot_start;
          std::string lag_msg;
          if (lag > std::chrono::hours(99)) {
            lag_msg = fmt::format(
                " (lag {} days)",
                std::chrono::duration_cast<std::chrono::hours>(lag).count()
                    / 24);
          } else if (lag > std::chrono::minutes(99)) {
            lag_msg = fmt::format(
                " (lag {} hr.)",
                std::chrono::duration_cast<std::chrono::hours>(lag).count());
          } else if (lag >= std::chrono::minutes(1)) {
            lag_msg = fmt::format(
                " (lag {} min.)",
                std::chrono::duration_cast<std::chrono::minutes>(lag).count());
          } else if (lag > slot_duration * 2) {
            lag_msg = " (lag <1 min.)";
          } else {
            lag_msg = fmt::format(
                " (lag {} ms.)",
                std::chrono::duration_cast<std::chrono::milliseconds>(lag)
                    .count());
          }

          auto now = std::chrono::steady_clock::now();

          self->logger_->info(
              "Imported block {} within {} ms.{}",
              block_info,
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  now - start_time)
                  .count(),
              lag_msg);

          auto const last_finalized_block =
              self->block_tree_->getLastFinalized();
          self->telemetry_->notifyBlockFinalized(last_finalized_block);
          auto current_best_block = self->block_tree_->bestBlock();
          self->telemetry_->notifyBlockImported(
              current_best_block, telemetry::BlockOrigin::kNetworkInitialSync);
          self->telemetry_->pushBlockStats();

          // Create new offchain worker for block if it is best only
          if (current_best_block.number > previous_best_block_number) {
            auto ocw_res = self->offchain_worker_api_->offchain_worker(
                block.header.parent_hash, block.header);
            if (ocw_res.has_failure()) {
              self->logger_->error(
                  "Can't spawn offchain worker for block {}: {}",
                  block_info,
                  ocw_res.error());
            }
          }

          callback(outcome::success());
        });
  }

}  // namespace kagome::consensus

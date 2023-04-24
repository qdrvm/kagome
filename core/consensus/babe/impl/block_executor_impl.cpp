/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/babe_error.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/block_appender_base.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "consensus/validation/block_validator.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "transaction_pool/transaction_pool.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

namespace {
  constexpr const char *kBlockExecutionTime =
      "kagome_block_verification_and_import_time";
}

namespace kagome::consensus::babe {

  BlockExecutorImpl::BlockExecutorImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      primitives::events::StorageSubscriptionEnginePtr storage_sub_engine,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      std::unique_ptr<BlockAppenderBase> appender)
      : block_tree_{std::move(block_tree)},
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
    BOOST_ASSERT(core_ != nullptr);
    BOOST_ASSERT(tx_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(offchain_worker_api_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    BOOST_ASSERT(telemetry_ != nullptr);
    BOOST_ASSERT(appender_ != nullptr);

    // Register metrics
    metrics_registry_->registerHistogramFamily(
        kBlockExecutionTime, "Time taken to verify and import blocks");
    metric_block_execution_time_ = metrics_registry_->registerHistogramMetric(
        kBlockExecutionTime,
        {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});
  }

  BlockExecutorImpl::~BlockExecutorImpl() = default;

  void BlockExecutorImpl::applyBlock(
      primitives::Block &&block,
      const std::optional<primitives::Justification> &justification,
      ApplyJustificationCb &&callback) {
    auto block_context = appender_->makeBlockContext(block.header);
    auto &block_info = block_context.block_info;
    if (auto header_res = block_tree_->getBlockHeader(block.header.parent_hash);
        header_res.has_error()
        && header_res.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      logger_->warn("Skipping a block {} with unknown parent", block_info);
      callback(BlockAdditionError::PARENT_NOT_FOUND);
      return;
    } else if (header_res.has_error()) {
      callback(header_res.as_failure());
      return;
    }

    // get current time to measure performance if block execution
    auto start_time = std::chrono::high_resolution_clock::now();

    bool block_was_applied_earlier = false;

    // check if block body already exists. If so, do not apply
    if (auto body_res =
            block_tree_->getBlockBody(block_context.block_info.hash);
        body_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing block: {}", block_info);

      if (auto res = block_tree_->addExistingBlock(
              block_context.block_info.hash, block.header);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }
      block_was_applied_earlier = true;
    } else if (body_res.error() != blockchain::BlockTreeError::BODY_NOT_FOUND) {
      callback(body_res.as_failure());
      return;
    }

    std::optional<ConsistencyGuard> consistency_guard{};
    if (auto res =
            appender_->observeDigestsAndValidateHeader(block, block_context);
        res.has_value()) {
      consistency_guard.emplace(std::move(res.value()));
    } else {
      callback(res.as_failure());
      return;
    }

    // Calculate best block before new one will be applied
    auto last_finalized_block = block_tree_->getLastFinalized();
    auto previous_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(previous_best_block_res.has_value());
    const auto &previous_best_block = previous_best_block_res.value();

    if (not block_was_applied_earlier) {
      auto exec_start = std::chrono::high_resolution_clock::now();

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

      if (auto res = core_->execute_block_ref(
              primitives::BlockReflection{
                  .header =
                      primitives::BlockHeaderReflection{
                          .parent_hash = block.header.parent_hash,
                          .number = block.header.number,
                          .state_root = block.header.state_root,
                          .extrinsics_root = block.header.extrinsics_root,
                          .digest = gsl::span<const primitives::DigestItem>(
                              block.header.digest.data(),
                              block.header.digest.size() - 1ull),
                      },
                  .body = block.body,
              },
              changes_tracker);
          res.has_error()) {
        callback(res.as_failure());
        return;
      }

      auto exec_end = std::chrono::high_resolution_clock::now();
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             exec_end - exec_start)
                             .count();
      SL_DEBUG(logger_, "Core_execute_block: {} ms", duration_ms);

      metric_block_execution_time_->observe(static_cast<double>(duration_ms)
                                            / 1000);

      // add block header if it does not exist
      if (auto res = block_tree_->addBlock(block); res.has_error()) {
        callback(res.as_failure());
        return;
      }

      changes_tracker->onBlockAdded(
          block_info.hash, storage_sub_engine_, chain_subscription_engine_);
    }

    /// TODO(iceseer): in a case we change the authority set, we can get an
    /// error with the following behavior: the finalisation will commit the
    /// authority change and the step of the next block processing will be
    /// failed because of VRF error
    BOOST_ASSERT(consistency_guard);
    consistency_guard->commit();

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
            auto hash = self->hasher_->blake2b_256(extrinsic.data);
            SL_DEBUG(self->logger_, "Contains extrinsic with hash: {}", hash);
            auto res = self->tx_pool_->removeOne(hash);
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
          }

          auto now = std::chrono::high_resolution_clock::now();

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
          auto current_best_block_res = self->block_tree_->getBestContaining(
              last_finalized_block.hash, std::nullopt);
          BOOST_ASSERT(current_best_block_res.has_value());
          const auto &current_best_block = current_best_block_res.value();
          self->telemetry_->notifyBlockImported(
              current_best_block, telemetry::BlockOrigin::kNetworkInitialSync);

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

}  // namespace kagome::consensus::babe

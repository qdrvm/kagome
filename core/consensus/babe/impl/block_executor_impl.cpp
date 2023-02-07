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
      std::unique_ptr<BlockAppenderBase> appender)
      : block_tree_{std::move(block_tree)},
        core_{std::move(core)},
        tx_pool_{std::move(tx_pool)},
        hasher_{std::move(hasher)},
        offchain_worker_api_(std::move(offchain_worker_api)),
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

  outcome::result<void> BlockExecutorImpl::applyBlock(
      primitives::Block &&block,
      std::optional<primitives::Justification> const &justification) {
    auto block_context = appender_->makeBlockContext(block.header);
    auto &block_info = block_context.block_info;
    if (auto header_res = block_tree_->getBlockHeader(block.header.parent_hash);
        header_res.has_error()
        && header_res.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      logger_->warn("Skipping a block {} with unknown parent", block_info);
      return BlockAdditionError::PARENT_NOT_FOUND;
    } else if (header_res.has_error()) {
      return header_res.as_failure();
    }

    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    bool block_was_applied_earlier = false;

    // check if block body already exists. If so, do not apply
    if (auto body_res =
            block_tree_->getBlockBody(block_context.block_info.hash);
        body_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing block: {}", block_info);

      OUTCOME_TRY(block_tree_->addExistingBlock(block_context.block_info.hash,
                                                block.header));
      block_was_applied_earlier = true;
    } else if (body_res.error() != blockchain::BlockTreeError::BODY_NOT_FOUND) {
      return body_res.as_failure();
    }

    OUTCOME_TRY(
        consistency_guard,
        appender_->observeDigestsAndValidateHeader(block, block_context));

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

      auto block_without_seal_digest = block;

      // block should be applied without last digest which contains the seal
      block_without_seal_digest.header.digest.pop_back();

      OUTCOME_TRY(core_->execute_block(block_without_seal_digest));

      auto exec_end = std::chrono::high_resolution_clock::now();
      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             exec_end - exec_start)
                             .count();
      SL_DEBUG(logger_, "Core_execute_block: {} ms", duration_ms);

      metric_block_execution_time_->observe(static_cast<double>(duration_ms)
                                            / 1000);

      // add block header if it does not exist
      OUTCOME_TRY(block_tree_->addBlock(block));
    }

    OUTCOME_TRY(appender_->applyJustifications(block_info, justification));

    // remove block's extrinsics from tx pool
    for (const auto &extrinsic : block.body) {
      auto hash = hasher_->blake2b_256(extrinsic.data);
      SL_DEBUG(logger_, "Contains extrinsic with hash: {}", hash);
      auto res = tx_pool_->removeOne(hash);
      if (res.has_error()
          && res
                 != outcome::failure(
                     transaction_pool::TransactionPoolError::TX_NOT_FOUND)) {
        return res.as_failure();
      }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    logger_->info(
        "Imported block {} within {} ms",
        block_info,
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
            .count());

    last_finalized_block = block_tree_->getLastFinalized();
    telemetry_->notifyBlockFinalized(last_finalized_block);
    auto current_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(current_best_block_res.has_value());
    const auto &current_best_block = current_best_block_res.value();
    telemetry_->notifyBlockImported(
        current_best_block, telemetry::BlockOrigin::kNetworkInitialSync);

    // Create new offchain worker for block if it is best only
    if (current_best_block.number > previous_best_block.number) {
      auto ocw_res = offchain_worker_api_->offchain_worker(
          block.header.parent_hash, block.header);
      if (ocw_res.has_failure()) {
        logger_->error("Can't spawn offchain worker for block {}: {}",
                       block_info,
                       ocw_res.error());
      }
    }

    consistency_guard.commit();

    return outcome::success();
  }

}  // namespace kagome::consensus::babe

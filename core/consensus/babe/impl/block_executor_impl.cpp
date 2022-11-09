/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/block_executor_impl.hpp"

#include <chrono>

#include "blockchain/block_tree_error.hpp"
#include "blockchain/digest_tracker.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/consistency_keeper.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/babe/impl/threshold_util.hpp"
#include "consensus/babe/types/slot.hpp"
#include "consensus/grandpa/voting_round_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "primitives/common.hpp"
#include "runtime/runtime_api/offchain_worker_api.hpp"
#include "scale/scale.hpp"
#include "transaction_pool/transaction_pool_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BlockExecutorImpl::Error, e) {
  using E = kagome::consensus::BlockExecutorImpl::Error;
  switch (e) {
    case E::INVALID_BLOCK:
      return "Invalid block";
    case E::PARENT_NOT_FOUND:
      return "Parent not found";
    case E::INTERNAL_ERROR:
      return "Internal error";
  }
  return "Unknown error";
}

namespace {
  constexpr const char *kBlockExecutionTime =
      "kagome_block_verification_and_import_time";
}

namespace kagome::consensus {

  BlockExecutorImpl::BlockExecutorImpl(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::Core> core,
      std::shared_ptr<consensus::babe::BabeConfigRepository> babe_config_repo,
      std::shared_ptr<BlockValidator> block_validator,
      std::shared_ptr<grandpa::Environment> grandpa_environment,
      std::shared_ptr<transaction_pool::TransactionPool> tx_pool,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
      std::shared_ptr<BabeUtil> babe_util,
      std::shared_ptr<runtime::OffchainWorkerApi> offchain_worker_api,
      std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper)
      : block_tree_{std::move(block_tree)},
        core_{std::move(core)},
        babe_config_repo_{std::move(babe_config_repo)},
        block_validator_{std::move(block_validator)},
        grandpa_environment_{std::move(grandpa_environment)},
        tx_pool_{std::move(tx_pool)},
        hasher_{std::move(hasher)},
        digest_tracker_(std::move(digest_tracker)),
        babe_util_(std::move(babe_util)),
        offchain_worker_api_(std::move(offchain_worker_api)),
        consistency_keeper_(std::move(consistency_keeper)),
        logger_{log::createLogger("BlockExecutor", "block_executor")},
        telemetry_{telemetry::createTelemetryService()} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(core_ != nullptr);
    BOOST_ASSERT(babe_config_repo_ != nullptr);
    BOOST_ASSERT(block_validator_ != nullptr);
    BOOST_ASSERT(grandpa_environment_ != nullptr);
    BOOST_ASSERT(tx_pool_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(digest_tracker_ != nullptr);
    BOOST_ASSERT(babe_util_ != nullptr);
    BOOST_ASSERT(offchain_worker_api_ != nullptr);
    BOOST_ASSERT(consistency_keeper_ != nullptr);
    BOOST_ASSERT(logger_ != nullptr);
    BOOST_ASSERT(telemetry_ != nullptr);

    // Register metrics
    metrics_registry_->registerHistogramFamily(
        kBlockExecutionTime, "Time taken to verify and import blocks");
    metric_block_execution_time_ = metrics_registry_->registerHistogramMetric(
        kBlockExecutionTime,
        {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10});
  }

  outcome::result<void> BlockExecutorImpl::applyBlock(
      primitives::BlockData &&b) {
    if (not b.header.has_value()) {
      logger_->warn("Skipping a block without header");
      return Error::INVALID_BLOCK;
    }
    auto &header = b.header.value();

    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());

    primitives::BlockInfo block_info(header.number, block_hash);

    if (auto header_res = block_tree_->getBlockHeader(header.parent_hash);
        header_res.has_error()
        && header_res.error() == blockchain::BlockTreeError::HEADER_NOT_FOUND) {
      logger_->warn("Skipping a block {} with unknown parent", block_info);
      return Error::PARENT_NOT_FOUND;
    } else if (header_res.has_error()) {
      return header_res.as_failure();
    }

    // get current time to measure performance if block execution
    auto t_start = std::chrono::high_resolution_clock::now();

    bool block_already_exists = false;

    // check if block body already exists. If so, do not apply
    if (auto body_res = block_tree_->getBlockBody(block_hash);
        body_res.has_value()) {
      SL_DEBUG(logger_, "Skip existing block: {}", block_info);

      OUTCOME_TRY(block_tree_->addExistingBlock(block_hash, header));
      block_already_exists = true;
    } else if (body_res.error() != blockchain::BlockTreeError::BODY_NOT_FOUND) {
      return body_res.as_failure();
    }

    if (not b.body.has_value()) {
      logger_->warn("Skipping a block without body.");
      return Error::INVALID_BLOCK;
    }
    auto &body = b.body.value();

    primitives::Block block{.header = std::move(header),
                            .body = std::move(body)};

    OUTCOME_TRY(babe_digests, getBabeDigests(block.header));

    const auto &babe_header = babe_digests.second;

    auto slot_number = babe_header.slot_number;

    babe_util_->syncEpoch([&] {
      auto res = block_tree_->getBlockHeader(primitives::BlockNumber(1));
      if (res.has_error()) {
        if (block.header.number == 1) {
          SL_TRACE(logger_,
                   "First block slot is {}: it is first block (at executing)",
                   slot_number);
          return std::tuple(slot_number, false);
        } else {
          SL_TRACE(logger_,
                   "First block slot is {}: no first block (at executing)",
                   babe_util_->getCurrentSlot());
          return std::tuple(babe_util_->getCurrentSlot(), false);
        }
      }

      const auto &first_block_header = res.value();
      auto babe_digest_res = consensus::getBabeDigests(first_block_header);
      BOOST_ASSERT_MSG(babe_digest_res.has_value(),
                       "Any non genesis block must contain babe digest");
      auto first_slot_number = babe_digest_res.value().second.slot_number;

      auto is_first_block_finalized =
          block_tree_->getLastFinalized().number > 0;

      SL_TRACE(
          logger_,
          "First block slot is {}: by {}finalized first block (at executing)",
          first_slot_number,
          is_first_block_finalized ? "" : "non-");
      return std::tuple(first_slot_number, is_first_block_finalized);
    });

    auto epoch_number = babe_util_->slotToEpoch(slot_number);

    SL_INFO(
        logger_,
        "Applying block {} ({} in slot {}, epoch {}, authority #{})",  //   .
        block_info,
        to_string(babe_header.slotType()),
        slot_number,
        epoch_number,
        babe_header.authority_index);

    auto consistency_guard = consistency_keeper_->start(block_info);

    // observe digest of block
    // (must be done strictly after block will be added)
    auto digest_tracking_res =
        digest_tracker_->onDigest(block_info, block.header.digest);
    if (digest_tracking_res.has_error()) {
      SL_ERROR(logger_,
               "Error while tracking digest of block {}: {}",
               block_info,
               digest_tracking_res.error());
      return digest_tracking_res.as_failure();
    }

    auto babe_config = babe_config_repo_->config(block_info, epoch_number);
    if (babe_config == nullptr) {
      return Error::INVALID_BLOCK;  // TODO Change to more appropriate error
    }

    SL_TRACE(logger_,
             "Actual epoch digest to apply block {} (slot {}, epoch {}). "
             "Randomness: {}",
             block_info,
             slot_number,
             epoch_number,
             babe_config->randomness);

    auto threshold = calculateThreshold(babe_config->leadership_rate,
                                        babe_config->authorities,
                                        babe_header.authority_index);

    OUTCOME_TRY(block_validator_->validateHeader(
        block.header,
        epoch_number,
        babe_config->authorities[babe_header.authority_index].id,
        threshold,
        *babe_config));

    auto block_without_seal_digest = block;

    // block should be applied without last digest which contains the seal
    block_without_seal_digest.header.digest.pop_back();

    auto parent = block_tree_->getBlockHeader(block.header.parent_hash).value();

    auto last_finalized_block = block_tree_->getLastFinalized();
    auto previous_best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(previous_best_block_res.has_value());
    const auto &previous_best_block = previous_best_block_res.value();

    if (not block_already_exists) {
      auto exec_start = std::chrono::high_resolution_clock::now();
      SL_DEBUG(logger_,
               "Execute block {}, state {}, a child of block {}, state {}",
               block_info,
               block.header.state_root,
               primitives::BlockInfo(parent.number, block.header.parent_hash),
               parent.state_root);

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

    // apply justification if any (must be done strictly after block will be
    // added and his consensus-digests will be handled)
    if (b.justification.has_value()) {
      SL_VERBOSE(logger_, "Justification received for block {}", block_info);

      // try to apply left in justification store values first
      if (not justifications_.empty()) {
        std::vector<primitives::BlockInfo> to_remove;
        for (const auto &[block_justified_for, justification] :
             justifications_) {
          auto res = applyJustification(block_justified_for, justification);
          if (res) {
            to_remove.push_back(block_justified_for);
          }
        }
        if (not to_remove.empty()) {
          for (const auto &item : to_remove) {
            justifications_.erase(item);
          }
        }
      }

      auto res = applyJustification(block_info, b.justification.value());
      if (res.has_error()) {
        if (res
            == outcome::failure(grandpa::VotingRoundError::NOT_ENOUGH_WEIGHT)) {
          justifications_.emplace(block_info, b.justification.value());
        } else {
          return res.as_failure();
        }
      } else {
        // safely could be remove if current justification applied successfully
        justifications_.clear();
      }
    }

    // remove block's extrinsics from tx pool
    for (const auto &extrinsic : block.body) {
      auto res = tx_pool_->removeOne(hasher_->blake2b_256(extrinsic.data));
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

  outcome::result<void> BlockExecutorImpl::applyJustification(
      const primitives::BlockInfo &block_info,
      const primitives::Justification &justification) {
    return grandpa_environment_->applyJustification(block_info, justification);
  }

}  // namespace kagome::consensus

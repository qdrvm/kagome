/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/proposer_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"
#include "scale/kagome_scale.hpp"

namespace {
  constexpr const char *kTransactionsIncludedInBlock =
      "kagome_proposer_number_of_transactions";
}

namespace kagome::authorship {

  ProposerImpl::ProposerImpl(
      std::shared_ptr<BlockBuilderFactory> block_builder_factory,
      std::shared_ptr<Clock> clock,
      std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          ext_sub_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo)
      : block_builder_factory_{std::move(block_builder_factory)},
        clock_{std::move(clock)},
        transaction_pool_{std::move(transaction_pool)},
        ext_sub_engine_{std::move(ext_sub_engine)},
        extrinsic_event_key_repo_{std::move(extrinsic_event_key_repo)} {
    BOOST_ASSERT(block_builder_factory_);
    BOOST_ASSERT(transaction_pool_);
    BOOST_ASSERT(ext_sub_engine_);
    BOOST_ASSERT(extrinsic_event_key_repo_);

    metrics_registry_->registerGaugeFamily(
        kTransactionsIncludedInBlock,
        "Number of transactions included in block");
    metric_tx_included_in_block_ =
        metrics_registry_->registerGaugeMetric(kTransactionsIncludedInBlock);
  }

  outcome::result<primitives::Block> ProposerImpl::propose(
      const primitives::BlockInfo &parent_block,
      std::optional<Clock::TimePoint> deadline,
      const primitives::InherentData &inherent_data,
      const primitives::Digest &inherent_digest,
      TrieChangesTrackerOpt changes_tracker) {
    OUTCOME_TRY(block_builder_mode,
                block_builder_factory_->make(
                    parent_block, inherent_digest, std::move(changes_tracker)));
    auto &[block_builder, mode] = block_builder_mode;

    // Retrieve and add the inherent extrinsics to the block
    auto inherent_xts_res = block_builder->getInherentExtrinsics(inherent_data);
    if (not inherent_xts_res) {
      logger_->error("BlockBuilder->inherent_extrinsics failed with error: {}",
                     inherent_xts_res.error());
      return inherent_xts_res.error();
    }
    const auto &inherent_xts = inherent_xts_res.value();

    // Add each inherent extrinsic to the block
    for (const auto &xt : inherent_xts) {
      SL_DEBUG(logger_, "Adding inherent extrinsic: {}", xt.data);
      auto inserted_res = block_builder->pushExtrinsic(xt);
      if (not inserted_res) {
        if (BlockBuilderError::EXHAUSTS_RESOURCES == inserted_res.error()) {
          SL_WARN(logger_,
                  "Dropping non-mandatory inherent extrinsic from overweight "
                  "block.");
        } else if (BlockBuilderError::BAD_MANDATORY == inserted_res.error()) {
          SL_ERROR(logger_,
                   "Mandatory inherent extrinsic returned error. Block cannot "
                   "be produced.");
          return inserted_res.error();
        } else {
          SL_ERROR(
              logger_,
              "Inherent extrinsic returned unexpected error: {}. Dropping.",
              inserted_res.error());
          return inserted_res.error();
        }
      }
    }

    std::vector<primitives::Transaction::Hash> included_hashes;
    if (mode == ExtrinsicInclusionMode::AllExtrinsics) {
      // Remove stale transactions from the transaction pool
      auto remove_res = transaction_pool_->removeStale(parent_block.number);
      if (remove_res.has_error()) {
        SL_ERROR(logger_,
                 "Stale transactions remove failure: {}, Parent is {}",
                 remove_res.error(),
                 parent_block);
      }

      /// TODO(iceseer): switch to callback case(this case is needed to make
      /// tests complete)

      // Retrieve ready transactions from the transaction pool
      std::vector<std::pair<primitives::Transaction::Hash,
                            std::shared_ptr<const primitives::Transaction>>>
          ready_txs = transaction_pool_->getReadyTransactions();

      bool transaction_pushed = false;
      bool hit_block_size_limit = false;

      auto skipped = 0;
      auto block_size_limit = kBlockSizeLimit;
      const auto kMaxVarintLength =
          9;  /// Max varint size in bytes when encoded
      // we move estimateBlockSize() out of the loop for optimization purposes.
      // to avoid varint bytes length recalculation which indicates extrinsics
      // quantity, we add the maximum varint length at once.
      auto block_size = block_builder->estimateBlockSize() + kMaxVarintLength;
      // at the moment block_size includes block headers and a counter to hold a
      // number of transactions to be pushed to the block

      size_t included_tx_count = 0;

      // Iterate through the ready transactions
      for (const auto &[hash, tx] : ready_txs) {
        // Check if the deadline has been reached
        if (deadline && clock_->now() >= deadline) {
          break;
        }

        // Estimate the size of the transaction
        auto estimate_tx_size = scale::encoded_size(tx->ext).value();

        // Check if adding the transaction would exceed the block size limit
        if (block_size + estimate_tx_size > block_size_limit) {
          if (skipped < kMaxSkippedTransactions) {
            ++skipped;
            SL_DEBUG(logger_,
                     "Transaction would overflow the block size limit, "
                     "but will try {} more transactions before quitting.",
                     kMaxSkippedTransactions - skipped);
            continue;
          }
          // Reached the block size limit, stop adding transactions
          SL_DEBUG(logger_,
                   "Reached block size limit, proceeding with proposing.");
          hit_block_size_limit = true;
          break;
        }

        // Add the transaction to the block
        SL_DEBUG(logger_, "Adding extrinsic: {}", tx->ext.data);
        auto inserted_res = block_builder->pushExtrinsic(tx->ext);
        if (not inserted_res) {
          if (BlockBuilderError::EXHAUSTS_RESOURCES == inserted_res.error()) {
            if (skipped < kMaxSkippedTransactions) {
              // Skip the transaction and continue with the next one
              ++skipped;
              SL_DEBUG(logger_,
                       "Block seems full, but will try {} more transactions "
                       "before quitting.",
                       kMaxSkippedTransactions - skipped);
            } else {
              // Maximum number of transactions reached, stop adding
              // transactions
              SL_DEBUG(logger_, "Block is full, proceed with proposing.");
              break;
            }
          } else {
            logger_->warn("Extrinsic {} was not added to the block. Reason: {}",
                          tx->ext.data,
                          inserted_res.error());
          }
        } else {
          // Transaction was successfully added to the block
          block_size += estimate_tx_size;
          transaction_pushed = true;
          ++included_tx_count;
          included_hashes.emplace_back(hash);
        }
      }

      // Set the number of included transactions in the block metric
      metric_tx_included_in_block_->set(included_tx_count);

      if (hit_block_size_limit and not transaction_pushed) {
        SL_WARN(
            logger_,
            "Hit block size limit of `{}` without including any transaction!",
            block_size_limit);
      }
    }

    // Create the block
    OUTCOME_TRY(block, block_builder->bake());

    // Remove the included transactions from the transaction pool
    for (const auto &hash : included_hashes) {
      auto removed_res = transaction_pool_->removeOne(hash);
      if (not removed_res) {
        logger_->error(
            "Can't remove extrinsic {} after adding to the block. Reason: {}",
            hash,
            removed_res.error());
      }
    }

    return block;
  }

}  // namespace kagome::authorship

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/proposer_impl.hpp"

#include "authorship/impl/block_builder_error.hpp"

namespace {
  constexpr const char *kTransactionsIncludedInBlock =
      "kagome_proposer_number_of_transactions";
}

namespace kagome::authorship {

  ProposerImpl::ProposerImpl(
      std::shared_ptr<BlockBuilderFactory> block_builder_factory,
      std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          ext_sub_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo)
      : block_builder_factory_{std::move(block_builder_factory)},
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
      const primitives::BlockNumber &parent_block_number,
      const primitives::InherentData &inherent_data,
      const primitives::Digest &inherent_digest) {
    OUTCOME_TRY(
        block_builder,
        block_builder_factory_->make(parent_block_number, inherent_digest));

    auto inherent_xts_res = block_builder->getInherentExtrinsics(inherent_data);
    if (not inherent_xts_res) {
      logger_->error("BlockBuilder->inherent_extrinsics failed with error: {}",
                     inherent_xts_res.error().message());
      return inherent_xts_res.error();
    }
    auto inherent_xts = inherent_xts_res.value();

    for (const auto &xt : inherent_xts) {
      SL_DEBUG(logger_, "Adding inherent extrinsic: {}", xt.data.toHex());
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
              inserted_res.error().message());
          return inserted_res.error();
        }
      }
    }

    auto remove_res = transaction_pool_->removeStale(parent_block_number);
    if (remove_res.has_error()) {
      SL_ERROR(
          logger_,
          "Stale transactions remove failure: {}, Parent block number is {}",
          remove_res.error().message(),
          parent_block_number);
    }
    const auto &ready_txs = transaction_pool_->getReadyTransactions();

    bool transaction_pushed = false;
    bool hit_block_size_limit = false;

    auto skipped = 0;
    auto block_size_limit = kBlockSizeLimit;
    const auto kMaxVarintLength = 9;  /// Max varint size in bytes when encoded
    // we move estimateBlockSize() out of the loop for optimization purposes.
    // to avoid varint bytes length recalculation which indicates extrinsics
    // quantity, we add the maximum varint length at once.
    auto block_size = block_builder->estimateBlockSize() + kMaxVarintLength;
    // at the moment block_size includes block headers and a counter to hold a
    // number of transactions to be pushed to the block

    size_t included_tx_count = 0;
    for (const auto &[hash, tx] : ready_txs) {
      const auto &tx_ref = tx;
      scale::ScaleEncoderStream s(true);
      s << tx_ref->ext;
      auto estimate_tx_size = s.size();

      if (block_size + estimate_tx_size > block_size_limit) {
        if (skipped < kMaxSkippedTransactions) {
          ++skipped;
          SL_DEBUG(logger_,
                   "Transaction would overflow the block size limit, but will "
                   "try {} more transactions before quitting.",
                   kMaxSkippedTransactions - skipped);
          continue;
        }
        SL_DEBUG(logger_,
                 "Reached block size limit, proceeding with proposing.");
        hit_block_size_limit = true;
        break;
      }

      SL_DEBUG(logger_, "Adding extrinsic: {}", tx_ref->ext.data.toHex());
      auto inserted_res = block_builder->pushExtrinsic(tx->ext);
      if (not inserted_res) {
        if (BlockBuilderError::EXHAUSTS_RESOURCES == inserted_res.error()) {
          if (skipped < kMaxSkippedTransactions) {
            ++skipped;
            SL_DEBUG(logger_,
                     "Block seems full, but will try {} more transactions "
                     "before quitting.",
                     kMaxSkippedTransactions - skipped);
          } else {  // maximum amount of txs is pushed
            SL_DEBUG(logger_, "Block is full, proceed with proposing.");
            break;
          }
        } else {  // any other error than exhausted resources
          logger_->warn("Extrinsic {} was not added to the block. Reason: {}",
                        tx->ext.data.toHex().substr(0, 8),
                        inserted_res.error().message());
        }
      } else {  // tx was pushed successfully
        block_size += estimate_tx_size;
        transaction_pushed = true;
        ++included_tx_count;
      }
    }
    metric_tx_included_in_block_->set(included_tx_count);

    if (hit_block_size_limit and not transaction_pushed) {
      SL_WARN(logger_,
              "Hit block size limit of `{}` without including any transaction!",
              block_size_limit);
    }

    OUTCOME_TRY(block, block_builder->bake());

    for (const auto &[hash, tx] : ready_txs) {
      auto removed_res = transaction_pool_->removeOne(hash);
      if (not removed_res) {
        logger_->error(
            "Can't remove extrinsic (hash={}) after adding to the block. "
            "Reason: {}",
            hash.toHex(),
            removed_res.error().message());
      }
    }

    return std::move(block);
  }

}  // namespace kagome::authorship

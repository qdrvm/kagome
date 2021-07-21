/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/proposer_impl.hpp"

namespace kagome::authorship {

  ProposerImpl::ProposerImpl(
      std::shared_ptr<BlockBuilderFactory> block_builder_factory,
      std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
      std::shared_ptr<runtime::BlockBuilder> r_block_builder,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          ext_sub_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          extrinsic_event_key_repo)
      : block_builder_factory_{std::move(block_builder_factory)},
        transaction_pool_{std::move(transaction_pool)},
        r_block_builder_{std::move(r_block_builder)},
        ext_sub_engine_{std::move(ext_sub_engine)},
        extrinsic_event_key_repo_{std::move(extrinsic_event_key_repo)} {
    BOOST_ASSERT(block_builder_factory_);
    BOOST_ASSERT(transaction_pool_);
    BOOST_ASSERT(r_block_builder_);
    BOOST_ASSERT(ext_sub_engine_);
    BOOST_ASSERT(extrinsic_event_key_repo_);
  }

  outcome::result<primitives::Block> ProposerImpl::propose(
      const primitives::BlockNumber &parent_block_number,
      const primitives::InherentData &inherent_data,
      const primitives::Digest &inherent_digest) {
    OUTCOME_TRY(
        block_builder,
        block_builder_factory_->create(parent_block_number, inherent_digest));

    auto inherent_xts_res =
        r_block_builder_->inherent_extrinsics(inherent_data);
    if (not inherent_xts_res) {
      logger_->error("BlockBuilder->inherent_extrinsics failed with error: {}",
                     inherent_xts_res.error().message());
      return inherent_xts_res.error();
    }
    auto inherent_xts = inherent_xts_res.value();

    auto log_push_warn = [this](const primitives::Extrinsic &xt,
                                std::string_view message) {
      logger_->warn("Extrinsic {} was not added to the block. Reason: {}",
                    xt.data.toHex().substr(0, 8),
                    message);
    };

    for (const auto &xt : inherent_xts) {
      SL_DEBUG(logger_, "Adding inherent extrinsic: {}", xt.data.toHex());
      auto inserted_res = block_builder->pushExtrinsic(xt);
      if (not inserted_res) {
        log_push_warn(xt, inserted_res.error().message());
        return inserted_res.error();
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

    for (const auto &[hash, tx] : ready_txs) {
      const auto &tx_ref = tx;
      SL_DEBUG(logger_, "Adding extrinsic: {}", tx_ref->ext.data.toHex());
      auto inserted_res = block_builder->pushExtrinsic(tx->ext);
      if (not inserted_res) {
        log_push_warn(tx->ext, inserted_res.error().message());
        continue;
      }
      if (tx->observed_id.has_value()) {
        extrinsic_event_key_repo_->upgradeTransaction(tx->observed_id.value(),
                                                      parent_block_number + 1,
                                                      inserted_res.value());
      }
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

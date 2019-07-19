/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authorship/impl/proposer_impl.hpp"

#include "common/time.hpp"

namespace kagome::authorship {

  ProposerImpl::ProposerImpl(
      std::shared_ptr<BlockBuilderFactory> block_builder_factory,
      std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
      std::shared_ptr<runtime::BlockBuilderApi> r_block_builder)
      : block_builder_factory_{std::move(block_builder_factory)},
        transaction_pool_{std::move(transaction_pool)},
        r_block_builder_{std::move(r_block_builder)} {}

  outcome::result<primitives::Block> ProposerImpl::propose(
      const primitives::BlockId &parent_block_id,
      const primitives::InherentData &inherent_data,
      const primitives::Digest &inherent_digest, time::time_t deadline) {
    OUTCOME_TRY(
        block_builder,
        block_builder_factory_->create(parent_block_id, inherent_digest));

    OUTCOME_TRY(inherent_xts,
                r_block_builder_->inherent_extrinsics(inherent_data));
    for (const auto &xt : inherent_xts) {
      auto inserted_res = block_builder->pushExtrinsic(xt);
      if (not inserted_res) {
        // log error
        return inserted_res.error();
      }
    }

    const auto &ready_txs = transaction_pool_->getReadyTransactions();
    for (const auto &tx : ready_txs) {
      auto now = kagome::time::now();
      if (now > deadline) {
        break;
      }
      auto inserted_res = block_builder->pushExtrinsic(tx.ext);
      if (not inserted_res) {
        // log error
        return inserted_res.error();
      }
    }

    return block_builder->bake();
  }

}  // namespace kagome::authorship

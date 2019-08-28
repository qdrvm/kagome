/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/impl/extrinsic_api_impl.hpp"

#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {
  ExtrinsicApiImpl::ExtrinsicApiImpl(
      sptr<runtime::TaggedTransactionQueue> api,
      sptr<transaction_pool::TransactionPool> pool,
      sptr<crypto::Hasher> hasher,
      sptr<blockchain::BlockTree> block_tree)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)} {
    BOOST_ASSERT_MSG(api_ != nullptr, "extrinsic api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree is nullptr");
  }

  outcome::result<common::Hash256> ExtrinsicApiImpl::submitExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    auto &&[block_number, _] = block_tree_->deepestLeaf();

    OUTCOME_TRY(res, api_->validate_transaction(block_number, extrinsic));

    return kagome::visit_in_place(
        res,
        [](const primitives::Invalid &) {
          return ExtrinsicApiError::INVALID_STATE_TRANSACTION;
        },
        [](const primitives::Unknown &) {
          return ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION;
        },
        [&](const primitives::Valid &v) -> outcome::result<common::Hash256> {
          // compose Transaction object
          common::Hash256 hash = hasher_->blake2b_256(extrinsic.data);
          size_t length = extrinsic.data.size();
          // TODO(yuraz): PRE-220 find out what value to use for this parameter
          // in substrate tests it is always true (except the case of
          // initialization check)
          bool should_propagate = true;

          primitives::Transaction transaction{extrinsic,
                                              length,
                                              hash,
                                              v.priority,
                                              v.longevity,
                                              v.requires,
                                              v.provides,
                                              should_propagate};

          // send to pool
          OUTCOME_TRY(pool_->submitOne(std::move(transaction)));

          return hash;
        });
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  ExtrinsicApiImpl::pendingExtrinsics() {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
  }

  outcome::result<std::vector<common::Hash256>>
  ExtrinsicApiImpl::removeExtrinsic(
      const std::vector<primitives::ExtrinsicKey> &keys) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
  }

}  // namespace kagome::api

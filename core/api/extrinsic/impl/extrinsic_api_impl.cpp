/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/extrinsic/impl/extrinsic_api_impl.hpp"

#include <boost/system/error_code.hpp>
#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::api {
  ExtrinsicApiImpl::ExtrinsicApiImpl(
      sptr<runtime::TaggedTransactionQueue> api,
      sptr<transaction_pool::TransactionPool> pool,
      sptr<crypto::Hasher> hasher,
      sptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieDb> trie_db)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        trie_db_{std::move(trie_db)},
        logger_{common::createLogger("ExtrinsicApi")} {
    BOOST_ASSERT_MSG(api_ != nullptr, "extrinsic api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree is nullptr");
    BOOST_ASSERT_MSG(trie_db_ != nullptr, "trie db is nullptr");
    BOOST_ASSERT_MSG(logger_ != nullptr, "logger is nullptr");
  }

  outcome::result<common::Hash256> ExtrinsicApiImpl::submitExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    auto state_before_validate = trie_db_->getRootHash();
    OUTCOME_TRY(res, api_->validate_transaction(extrinsic));
    trie_db_->resetState(state_before_validate);

    return visit_in_place(
        res,
        [&](const primitives::TransactionValidityError &e) {
          return visit_in_place(
              e,
              // return either invalid or unknown validity error
              [](const auto &validity_error)
                  -> outcome::result<common::Hash256> {
                return validity_error;
              });
        },
        [&](const primitives::ValidTransaction &v)
            -> outcome::result<common::Hash256> {
          // compose Transaction object
          common::Hash256 hash = hasher_->blake2b_256(extrinsic.data);
          size_t length = extrinsic.data.size();

          primitives::Transaction transaction{extrinsic,
                                              length,
                                              hash,
                                              v.priority,
                                              v.longevity,
                                              v.requires,
                                              v.provides,
                                              v.propagate};

          // send to pool
          OUTCOME_TRY(pool_->submitOne(std::move(transaction)));

          return hash;
        });
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  ExtrinsicApiImpl::pendingExtrinsics() {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }

  outcome::result<std::vector<common::Hash256>>
  ExtrinsicApiImpl::removeExtrinsic(
      const std::vector<primitives::ExtrinsicKey> &keys) {
    BOOST_ASSERT_MSG(false, "not implemented");  // NOLINT
    return outcome::failure(boost::system::error_code{});
  }

}  // namespace kagome::api

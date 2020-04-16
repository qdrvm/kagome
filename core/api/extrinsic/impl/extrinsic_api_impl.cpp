/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
      sptr<blockchain::BlockTree> block_tree)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)},
        block_tree_{std::move(block_tree)},
        logger_{common::createLogger("ExtrinsicApi")} {
    BOOST_ASSERT_MSG(api_ != nullptr, "extrinsic api is nullptr");
    BOOST_ASSERT_MSG(pool_ != nullptr, "transaction pool is nullptr");
    BOOST_ASSERT_MSG(hasher_ != nullptr, "hasher is nullptr");
    BOOST_ASSERT_MSG(block_tree_ != nullptr, "block tree is nullptr");
    BOOST_ASSERT_MSG(logger_ != nullptr, "logger is nullptr");
  }

  outcome::result<common::Hash256> ExtrinsicApiImpl::submitExtrinsic(
      const primitives::Extrinsic &extrinsic) {
    OUTCOME_TRY(res, api_->validate_transaction(extrinsic));

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

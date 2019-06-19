/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/extrinsic_submission_api.hpp"

#include "primitives/transaction.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::service {
  ExtrinsicSubmissionApi::ExtrinsicSubmissionApi(
      sptr<runtime::TaggedTransactionQueue> api,
      sptr<transaction_pool::TransactionPool> pool, sptr<hash::Hasher> hasher)
      : api_{std::move(api)},
        pool_{std::move(pool)},
        hasher_{std::move(hasher)} {}

  outcome::result<common::Hash256> ExtrinsicSubmissionApi::submit_extrinsic(
      const common::Buffer &bytes) {
    // validate
    OUTCOME_TRY(res, api_->validate_transaction(primitives::Extrinsic{bytes}));

    return kagome::visit_in_place(
        res,
        [&](const primitives::Invalid &v) {
          // send response
          return ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION;
        },
        [&](const primitives::Unknown &v) {
          return ExtrinsicSubmissionError::UNKNOWN_STATE_TRANSACTION;
        },
        [&](const primitives::Valid &v) -> outcome::result<common::Hash256> {
          // compose Transaction
          common::Hash256 hash = hasher_->blake2_256(bytes);
          common::Buffer buffer_hash(hash);  // make hash parameter
          size_t length = bytes.size();      // find out what is length
          bool should_propagate = false;     // find out what is this value

          primitives::Transaction transaction{
              {bytes},     length,     buffer_hash, v.priority,
              v.longevity, v.requires, v.provides,  should_propagate};

          // send to pool
          OUTCOME_TRY(pool_->submitOne(transaction));

          return outcome::success();
        });
  }

  outcome::result<std::vector<std::vector<uint8_t>>>
  ExtrinsicSubmissionApi::pending_extrinsics() {
    // not implemented yet
    std::terminate();
  }

  outcome::result<std::vector<common::Hash256>>
  ExtrinsicSubmissionApi::remove_extrinsic(
      const std::vector<boost::variant<std::vector<uint8_t>, common::Hash256>>
          &bytes_or_hash) {
    // not implemented yet
    std::terminate();
  }

  void ExtrinsicSubmissionApi::watch_extrinsic(
      const primitives::Metadata &metadata,
      const primitives::Subscriber &subscriber, const common::Buffer &data) {
    // not implemented yet
    std::terminate();
  }

  outcome::result<bool> ExtrinsicSubmissionApi::unwatch_extrinsic(
      const std::optional<primitives::Metadata> &metadata,
      const primitives::SubscriptionId &id) {
    // not implemented yet
    std::terminate();
  }

}  // namespace kagome::service

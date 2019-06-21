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
      const primitives::Extrinsic &extrinsic) {
    // validate transaction
    OUTCOME_TRY(res, api_->validate_transaction(extrinsic));

    return kagome::visit_in_place(
        res,
        [](const primitives::Invalid &) {
          return ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION;
        },
        [](const primitives::Unknown &) {
          return ExtrinsicSubmissionError::UNKNOWN_STATE_TRANSACTION;
        },
        [&](const primitives::Valid &v) -> outcome::result<common::Hash256> {
          // compose Transaction object
          common::Hash256 hash = hasher_->blake2_256(extrinsic.data);
          common::Buffer buffer_hash(hash);
          size_t length = extrinsic.data.size();
          // TODO(yuraz): PRE-220 find out what value to use for this parameter
          // in substrate tests it is always true (except the case of initialization check)
          bool should_propagate = true;

          primitives::Transaction transaction{
              extrinsic,   length,     buffer_hash, v.priority,
              v.longevity, v.requires, v.provides,  should_propagate};

          // send to pool
          OUTCOME_TRY(pool_->submitOne(std::move(transaction)));

          return hash;
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

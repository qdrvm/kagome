/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/tagged_transaction_queue.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmTaggedTransactionQueue::WavmTaggedTransactionQueue(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::TransactionValidity>
  WavmTaggedTransactionQueue::validate_transaction(
      primitives::TransactionSource source,
      const primitives::Extrinsic &ext) {
    return executor_->callAtLatest<primitives::TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction", ext);
  }

}  // namespace kagome::runtime::wavm

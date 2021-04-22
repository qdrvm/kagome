/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_TAGGED_TRANSACTION_QUEUE_HPP
#define KAGOME_RUNTIME_WAVM_TAGGED_TRANSACTION_QUEUE_HPP

#include "runtime/tagged_transaction_queue.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmTaggedTransactionQueue final: public TaggedTransactionQueue {
   public:
    WavmTaggedTransactionQueue(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<primitives::TransactionValidity>
    validate_transaction(primitives::TransactionSource source,
                        const primitives::Extrinsic &ext) override {
      return executor_->call<primitives::TransactionValidity>(
          "TaggedTransactionQueue_validate_transaction", ext);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

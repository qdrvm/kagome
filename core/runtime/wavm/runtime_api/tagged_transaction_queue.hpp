/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_TAGGED_TRANSACTION_QUEUE_HPP
#define KAGOME_RUNTIME_WAVM_TAGGED_TRANSACTION_QUEUE_HPP

#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmTaggedTransactionQueue final: public TaggedTransactionQueue {
   public:
    WavmTaggedTransactionQueue(std::shared_ptr<Executor> executor);

    outcome::result<primitives::TransactionValidity>
    validate_transaction(primitives::TransactionSource source,
                        const primitives::Extrinsic &ext) override;

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

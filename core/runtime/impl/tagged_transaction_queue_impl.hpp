/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

#include <outcome/outcome.hpp>
#include "runtime/impl/wasm_executor.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class TaggedTransactionQueueImpl : public TaggedTransactionQueue {
   public:
    TaggedTransactionQueueImpl(common::Buffer state_code,
                               std::shared_ptr<extensions::Extension> extension);

    outcome::result<primitives::TransactionValidity> validate_transaction(
        const primitives::Extrinsic &ext) override;

   private:
    std::shared_ptr<WasmMemory> memory_;
    WasmExecutor executor_;
    common::Buffer state_code_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

#include "extensions/extension.hpp"
#include "runtime/impl/runtime_api.hpp"
#include "runtime/tagged_transaction_queue.hpp"
#include "runtime/wasm_provider.hpp"

namespace kagome::runtime {
  class TaggedTransactionQueueImpl : public RuntimeApi,
                                     public TaggedTransactionQueue {
   public:
    TaggedTransactionQueueImpl(
        const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
        const std::shared_ptr<extensions::Extension> &extension);

    ~TaggedTransactionQueueImpl() override = default;

    outcome::result<primitives::TransactionValidity> validate_transaction(
        primitives::BlockNumber number,
        const primitives::Extrinsic &ext) override;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_IMPL_HPP

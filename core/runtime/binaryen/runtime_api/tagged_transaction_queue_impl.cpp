/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::TransactionValidity;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      std::shared_ptr<WasmProvider> wasm_provider,
      const std::shared_ptr<RuntimeManager> &runtime_manager)
      : RuntimeApi(std::move(wasm_provider), runtime_manager) {}

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      const primitives::Extrinsic &ext) {
    return execute<TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction",
        CallPersistency::EPHEMERAL,
        ext);
  }
}  // namespace kagome::runtime::binaryen

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"

namespace kagome::runtime::binaryen {
  using primitives::TransactionValidity;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      const std::shared_ptr<runtime::WasmProvider> &wasm_provider,
      const std::shared_ptr<extensions::Extension> &extension)
      : RuntimeApi(wasm_provider, extension) {}

  outcome::result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      primitives::BlockNumber number, const primitives::Extrinsic &ext) {
    return execute<TransactionValidity>(
        "TaggedTransactionQueue_validate_transaction", number, ext);
  }
}  // namespace kagome::runtime::binaryen

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::runtime {
  using outcome::result;
  using scale::ScaleDecoderStream;

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension)
      : memory_(extension->memory()),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      const primitives::Extrinsic &ext) {
    OUTCOME_TRY(encoded_ext, scale::encode(ext));

    // TODO (Harrm) PRE-98: after check for memory overflow is done, refactor it
    runtime::SizeType ext_size = encoded_ext.size();
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, common::Buffer(encoded_ext));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_,
                       "TaggedTransactionQueue_validate_transaction", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<primitives::TransactionValidity>(buffer);
  }

}  // namespace kagome::runtime

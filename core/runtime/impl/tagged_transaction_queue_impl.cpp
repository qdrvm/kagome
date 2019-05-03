/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/tagged_transaction_queue_impl.hpp"

#include "runtime/impl/wasm_memory_stream.hpp"

using outcome::result;

namespace kagome::runtime {

  TaggedTransactionQueueImpl::TaggedTransactionQueueImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension,
      std::shared_ptr<primitives::ScaleCodec> codec)
      : memory_(extension->memory()),
        codec_(std::move(codec)),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  result<primitives::TransactionValidity>
  TaggedTransactionQueueImpl::validate_transaction(
      const primitives::Extrinsic &ext) {
    OUTCOME_TRY(encoded_ext, codec_->encodeExtrinsic(ext));

    runtime::SizeType ext_size = encoded_ext.size();
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, encoded_ext);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_,
                       "TaggedTransactionQueue_validate_transaction", ll));

    uint32_t res_addr = res.geti64() >> 32;

    WasmMemoryStream s(memory_);
    s.advance(res_addr);

    return codec_->decodeTransactionValidity(s);
  }

}  // namespace kagome::runtime

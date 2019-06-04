/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/offchain_worker_impl.hpp"

#include "runtime/impl/runtime_external_interface.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::BlockNumber;

  OffchainWorkerImpl::OffchainWorkerImpl(
      common::Buffer state_code,
      std::shared_ptr<extensions::Extension> extension)
      : state_code_(std::move(state_code)),
        memory_(extension->memory()),
        executor_(std::move(extension)) {}

  outcome::result<void> OffchainWorkerImpl::offchain_worker(BlockNumber bn) {
    OUTCOME_TRY(params, scale::encode(bn));
    runtime::SizeType ext_size = params.size();
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, Buffer(params));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "OffchainWorkerApi_offchain_worker", ll));

    return outcome::success();
  }

}  // namespace kagome::runtime

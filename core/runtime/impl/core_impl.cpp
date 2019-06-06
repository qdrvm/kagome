/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

#include "runtime/impl/runtime_external_interface.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {
  using common::Buffer;

  CoreImpl::CoreImpl(common::Buffer state_code,
                     std::shared_ptr<extensions::Extension> extension)
      : state_code_(std::move(state_code)),
        memory_(extension->memory()),
        executor_(std::move(extension)) {}

  outcome::result<primitives::Version> CoreImpl::version() {
    OUTCOME_TRY(version_long,
                executor_.call(
                    state_code_, "Core_version",
                    wasm::LiteralList({wasm::Literal(0), wasm::Literal(0)})));

    runtime::WasmPointer param_addr = getWasmAddr(version_long.geti64());
    runtime::SizeType param_len = getWasmLen(version_long.geti64());
    auto buffer = memory_->loadN(param_addr, param_len);

    return scale::decode<primitives::Version>(buffer);
  }

  outcome::result<void> CoreImpl::execute_block(
      const kagome::primitives::Block &block) {
    OUTCOME_TRY(encoded_block, scale::encode(block));

    runtime::SizeType block_size = encoded_block.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(block_size);
    memory_->storeBuffer(ptr, Buffer(encoded_block));

    OUTCOME_TRY(executor_.call(
        state_code_, "Core_execute_block",
        wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(block_size)})));

    return outcome::success();
  }

  outcome::result<void> CoreImpl::initialise_block(
      const kagome::primitives::BlockHeader &header) {
    OUTCOME_TRY(encoded_header, scale::encode(header));

    runtime::SizeType header_size = encoded_header.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(header_size);
    memory_->storeBuffer(ptr, Buffer(encoded_header));

    OUTCOME_TRY(executor_.call(
        state_code_, "Core_initialise_block",
        wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(header_size)})));

    return outcome::success();
  }

  outcome::result<std::vector<primitives::AuthorityId>>
  CoreImpl::authorities() {
    OUTCOME_TRY(result_long,
                executor_.call(
                    state_code_, "Core_authorities",
                    wasm::LiteralList({wasm::Literal(0), wasm::Literal(0)})));

    runtime::WasmPointer authority_address = getWasmAddr(result_long.geti64());
    runtime::SizeType len = getWasmLen(result_long.geti64());
    auto buffer = memory_->loadN(authority_address, len);

    return scale::decode<std::vector<primitives::AuthorityId>>(buffer);
  }

}  // namespace kagome::runtime

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/core_impl.hpp"

#include "primitives/scale_codec.hpp"
#include "runtime/impl/runtime_external_interface.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "runtime/impl/wasm_memory_stream.hpp"

namespace kagome::runtime {

  CoreImpl::CoreImpl(common::Buffer state_code,
                     std::shared_ptr<extensions::Extension> extension,
                     std::shared_ptr<primitives::ScaleCodec> codec)
      : state_code_(std::move(state_code)),
        memory_(extension->memory()),
        executor_(std::move(extension)),
        codec_(std::move(codec)) {}

  outcome::result<primitives::Version> CoreImpl::version() {
    OUTCOME_TRY(version_long,
                executor_.call(
                    state_code_, "Core_version",
                    wasm::LiteralList({wasm::Literal(0), wasm::Literal(0)})));

    runtime::WasmPointer version_address = getWasmAddr(version_long.geti64());

    WasmMemoryStream stream(memory_);

    OUTCOME_TRY(stream.advance(version_address));

    OUTCOME_TRY(version, codec_->decodeVersion(stream));

    return std::move(version);
  }

  outcome::result<void> CoreImpl::execute_block(
      const kagome::primitives::Block &block) {
    OUTCOME_TRY(encoded_block, codec_->encodeBlock(block));

    runtime::SizeType block_size = encoded_block.size();
    runtime::WasmPointer ptr = memory_->allocate(block_size);
    memory_->storeBuffer(ptr, encoded_block);

    OUTCOME_TRY(executor_.call(
        state_code_, "Core_execute_block",
        wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(block_size)})));

    return outcome::success();
  }

  outcome::result<void> CoreImpl::initialise_block(
      const kagome::primitives::BlockHeader &header) {
    OUTCOME_TRY(encoded_header, codec_->encodeBlockHeader(header));

    runtime::SizeType header_size = encoded_header.size();
    runtime::WasmPointer ptr = memory_->allocate(header_size);
    memory_->storeBuffer(ptr, encoded_header);

    OUTCOME_TRY(executor_.call(
        state_code_, "Core_initialise_block",
        wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(header_size)})));

    return outcome::success();
  }

  outcome::result<std::vector<primitives::AuthorityId>> CoreImpl::authorities(
      primitives::BlockId block_id) {
    OUTCOME_TRY(encoded_id, codec_->encodeBlockId(block_id));

    runtime::SizeType id_size = encoded_id.size();
    runtime::WasmPointer ptr = memory_->allocate(id_size);
    memory_->storeBuffer(ptr, encoded_id);

    OUTCOME_TRY(result_long,
                executor_.call(state_code_, "Core_authorities",
                               wasm::LiteralList({wasm::Literal(ptr),
                                                  wasm::Literal(id_size)})));

    runtime::WasmPointer authority_address = getWasmAddr(result_long.geti64());

    WasmMemoryStream stream(memory_);
    OUTCOME_TRY(stream.advance(authority_address));

    return codec_->decodeAuthorityIds(stream);
  }

}  // namespace kagome::runtime

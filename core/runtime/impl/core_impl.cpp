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
                     std::shared_ptr<primitives::ScaleCodec> codec,
                     common::Logger logger)
      : state_code_(std::move(state_code)),
        memory_(extension->memory()),
        executor_(std::move(extension)),
        codec_(std::move(codec)),
        logger_(std::move(logger)) {}

  outcome::result<primitives::Version> CoreImpl::version() {
    uint64_t version_long =
        executor_
            .call(state_code_.toVector(), "Core_version",
                  wasm::LiteralList({wasm::Literal(0), wasm::Literal(0)}))
            .geti64();

    // first 32 bits are address and second are length (length not neededS)
    runtime::WasmPointer version_address = (version_long & 0xFFFFFFFFLL);

    WasmMemoryStream stream(memory_);

    auto advance_result = stream.advance(version_address);
    if (not advance_result) {
      return advance_result.error();
    }

    auto version = codec_->decodeVersion(stream);

    if (not version) {
      return version.error();
    }

    return version.value();
  }

  outcome::result<void> CoreImpl::execute_block(
      const kagome::primitives::Block &block) {
    auto encoded_block_result = codec_->encodeBlock(block);

    if (not encoded_block_result) {
      return encoded_block_result.error();
    }

    runtime::SizeType block_size = encoded_block_result.value().size();
    runtime::WasmPointer ptr = memory_->allocate(block_size);
    memory_->storeBuffer(ptr, encoded_block_result.value());

    uint64_t result_long =
        executor_
            .call(state_code_.toVector(), "Core_execute_block",
                  wasm::LiteralList(
                      {wasm::Literal(ptr), wasm::Literal(block_size)}))
            .geti64();

    return outcome::success();
  }

  outcome::result<void> CoreImpl::initialise_block(
      const kagome::primitives::BlockHeader &header) {
    auto encoded_header_result = codec_->encodeBlockHeader(header);

    if (not encoded_header_result) {
      return encoded_header_result.error();
    }

    runtime::SizeType header_size = encoded_header_result.value().size();
    runtime::WasmPointer ptr = memory_->allocate(header_size);
    memory_->storeBuffer(ptr, encoded_header_result.value());

    executor_
        .call(
            state_code_.toVector(), "Core_initialise_block",
            wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(header_size)}))
        .geti64();

    return outcome::success();
  }

  outcome::result<std::vector<primitives::AuthorityId>> CoreImpl::authorities(
      primitives::BlockId block_id) {
    auto encoded_id_result = codec_->encodeBlockId(block_id);

    if (not encoded_id_result) {
      return encoded_id_result.error();
    }

    runtime::SizeType id_size = encoded_id_result.value().size();
    runtime::WasmPointer ptr = memory_->allocate(id_size);
    memory_->storeBuffer(ptr, encoded_id_result.value());

    uint64_t result_long =
        executor_
            .call(
                state_code_.toVector(), "Core_authorities",
                wasm::LiteralList({wasm::Literal(ptr), wasm::Literal(id_size)}))
            .geti64();

    // first 32 bits are address and second are the length (length not needed)
    runtime::WasmPointer authority_address = (result_long & 0xFFFFFFFFLL);

    WasmMemoryStream stream(memory_);
    auto advance_result = stream.advance(authority_address);
    if (not advance_result) {
      return advance_result.error();
    }

    return codec_->decodeAuthorityIds(stream);
  }

}  // namespace kagome::runtime

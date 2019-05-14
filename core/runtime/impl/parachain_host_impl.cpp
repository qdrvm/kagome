/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

#include "runtime/impl/wasm_memory_stream.hpp"
#include "scale/collection.hpp"
#include "scale/optional.hpp"
#include "scale/buffer_codec.hpp"
#include "scale/blob_codec.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;
  using scale::fixedwidth::encodeUint32;
  using scale::optional::decodeOptional;

  ParachainHostImpl::ParachainHostImpl(
      common::Buffer state_code,  // find out what is it
      std::shared_ptr<extensions::Extension> extension,
      std::shared_ptr<primitives::ScaleCodec> codec)
      : memory_(extension->memory()),
        codec_(std::move(codec)),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<DutyRoster> ParachainHostImpl::dutyRoster() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_duty_roster", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return codec_->decodeDutyRoster(s);
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::activeParachains() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "ParachainHost_active_parachains", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::collection::decodeCollection<ParaId>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainHead(
      ParachainId id) {
    Buffer params;
    encodeUint32(id, params);

    runtime::SizeType ext_size = params.size();
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, params);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_head", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::optional::decodeOptional<Buffer>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    Buffer params;
    encodeUint32(id, params);

    runtime::SizeType ext_size = params.size();
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, params);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_code", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::optional::decodeOptional<Buffer>(s);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators(
      primitives::BlockId block_id) {
    OUTCOME_TRY(encoded_id, codec_->encodeBlockId(block_id));

    runtime::SizeType id_size = encoded_id.size();
    runtime::WasmPointer ptr = memory_->allocate(id_size);
    memory_->storeBuffer(ptr, encoded_id);

    OUTCOME_TRY(res,
                executor_.call(state_code_, "Core_authorities",
                               wasm::LiteralList({wasm::Literal(ptr),
                                                  wasm::Literal(id_size)})));

    // first 32 bits are address and second are the length (length not needed)
    runtime::WasmPointer authority_address =
        static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream stream(memory_);
    OUTCOME_TRY(stream.advance(authority_address));

    return scale::collection::decodeCollection<ValidatorId>(stream);
  }

}  // namespace kagome::runtime

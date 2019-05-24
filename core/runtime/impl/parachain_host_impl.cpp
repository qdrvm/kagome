/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

#include "runtime/impl/wasm_memory_stream.hpp"
#include "scale/blob_codec.hpp"
#include "scale/buffer_codec.hpp"
#include "scale/collection.hpp"
#include "scale/optional.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;
  using scale::fixedwidth::encodeUint32;
  using scale::optional::decodeOptional;
  using scale::ScaleEncoderStream;

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

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return codec_->decodeDutyRoster(s);
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::activeParachains() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "ParachainHost_active_parachains", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::collection::decodeCollection<ParaId>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainHead(
      ParachainId id) {
    ScaleEncoderStream se;
    se << id;
    auto && params = se.getBuffer();

    runtime::SizeType ext_size = params.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, params);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_head", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::optional::decodeOptional<Buffer>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    ScaleEncoderStream se;
    se << id;
    auto && params = se.getBuffer();

    runtime::SizeType ext_size = params.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, params);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_code", ll));

    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return scale::optional::decodeOptional<Buffer>(s);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};
    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_validators", ll));

    // first 32 bits are address and second are the length (length not needed)
    uint32_t res_addr = getWasmAddr(res.geti64());

    WasmMemoryStream stream(memory_);
    OUTCOME_TRY(stream.advance(res_addr));

    return scale::collection::decodeCollection<ValidatorId>(stream);
  }

}  // namespace kagome::runtime

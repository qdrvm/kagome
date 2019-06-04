/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/parachain_host_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::parachain::DutyRoster;
  using primitives::parachain::ParaId;
  using primitives::parachain::ValidatorId;
  using scale::ScaleDecoderStream;
  using scale::encode;
  using scale::decode;

  ParachainHostImpl::ParachainHostImpl(
      common::Buffer state_code,  // find out what is it
      std::shared_ptr<extensions::Extension> extension)
      : memory_(extension->memory()),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_duty_roster", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());

    auto buffer = memory_->loadN(res_addr, len);
    ScaleDecoderStream s(buffer);

    return decode<DutyRoster>(s);
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::active_parachains() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "ParachainHost_active_parachains", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    ScaleDecoderStream s(buffer);

    return decode<std::vector<ParaId>>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachain_head(
      ParachainId id) {
    OUTCOME_TRY(params, encode(id));

    runtime::SizeType ext_size = params.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, Buffer(params));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_head", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    ScaleDecoderStream s(buffer);

    return decode<std::optional<Buffer>>(s);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    OUTCOME_TRY(params, encode(id));

    runtime::SizeType ext_size = params.size();
    // TODO (yuraz): PRE-98 after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, Buffer(params));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "ParachainHost_parachain_code", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);
    ScaleDecoderStream s(buffer);

    return decode<std::optional<Buffer>>(s);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};
    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_validators", ll));

    // first 32 bits are address and second are the length (length not needed)
    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());

    auto buffer = memory_->loadN(res_addr, len);
    ScaleDecoderStream stream(buffer);

    return decode<std::vector<ValidatorId>>(stream);
  }

}  // namespace kagome::runtime

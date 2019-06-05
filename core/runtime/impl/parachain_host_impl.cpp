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

  ParachainHostImpl::ParachainHostImpl(
      common::Buffer state_code,  // find out what is it
      std::shared_ptr<extensions::Extension> extension)
      : memory_(extension->memory()),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<DutyRoster> ParachainHostImpl::dutyRoster() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_duty_roster", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<DutyRoster>(buffer);
  }

  outcome::result<std::vector<ParaId>> ParachainHostImpl::activeParachains() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "ParachainHost_active_parachains", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<std::vector<ParaId>>(buffer);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainHead(
      ParachainId id) {
    OUTCOME_TRY(params, scale::encode(id));

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

    return scale::decode<std::optional<Buffer>>(buffer);
  }

  outcome::result<std::optional<Buffer>> ParachainHostImpl::parachainCode(
      ParachainId id) {
    OUTCOME_TRY(params, scale::encode(id));

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

    return scale::decode<std::optional<Buffer>>(buffer);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};
    OUTCOME_TRY(res,
                executor_.call(state_code_, "ParachainHost_validators", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    return scale::decode<std::vector<ValidatorId>>(buffer);
  }

}  // namespace kagome::runtime

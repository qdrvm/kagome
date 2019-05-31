/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/grandpa_impl.hpp"

#include "scale/scale.hpp"
#include "scale/scale_error.hpp"
#include "common/blob.hpp"

namespace kagome::runtime {
  using common::Buffer;
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;
  using primitives::WeightedAuthority;
  using scale::ScaleDecoderStream;

  GrandpaImpl::GrandpaImpl(common::Buffer state_code,
                           std::shared_ptr<extensions::Extension> extension)
      : memory_(extension->memory()),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<std::optional<ScheduledChange>> GrandpaImpl::pending_change(
      const Grandpa::Digest &digest) {
    OUTCOME_TRY(encoded_digest, scale::encode(digest));
    runtime::SizeType ext_size = encoded_digest.size();
    // TODO (yuraz) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, Buffer(encoded_digest));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "GrandpaApi_grandpa_pending_change", ll));

    runtime::WasmPointer res_addr = getWasmAddr(res.geti64());
    runtime::SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);
    ScaleDecoderStream s(gsl::make_span(buffer.toVector()));

    return scale::decode<std::optional<ScheduledChange>>(s);
  }

  outcome::result<std::optional<ForcedChange>> GrandpaImpl::forced_change(
      const Grandpa::Digest &digest) {
    OUTCOME_TRY(encoded_digest, scale::encode(digest));

    runtime::SizeType ext_size = encoded_digest.size();
    // TODO (yuraz) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, Buffer(encoded_digest));

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "GrandpaApi_grandpa_forced_change", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len = getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);
    ScaleDecoderStream s(gsl::make_span(buffer.toVector()));

    return scale::decode<std::optional<ForcedChange>>(s);
  }

  outcome::result<std::vector<WeightedAuthority>> GrandpaImpl::authorities() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "GrandpaApi_grandpa_authorities", ll));

    WasmPointer res_addr = getWasmAddr(res.geti64());
    SizeType len= getWasmLen(res.geti64());
    auto buffer = memory_->loadN(res_addr, len);

    ScaleDecoderStream s(gsl::make_span(buffer.toVector()));

    return scale::decode<std::vector<WeightedAuthority>>(s);
  }
}  // namespace kagome::runtime

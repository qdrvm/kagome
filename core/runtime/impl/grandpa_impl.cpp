/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/grandpa_impl.hpp"

#include "runtime/impl/wasm_memory_stream.hpp"
#include "scale/collection.hpp"
#include "scale/optional.hpp"
#include "scale/scale_error.hpp"

namespace kagome::runtime {
  using primitives::Digest;
  using primitives::ForcedChange;
  using primitives::ScheduledChange;
  using primitives::SessionKey;
  using primitives::WeightedAuthority;

  GrandpaImpl::GrandpaImpl(common::Buffer state_code,
                           std::shared_ptr<extensions::Extension> extension,
                           std::shared_ptr<primitives::ScaleCodec> codec)
      : memory_(extension->memory()),
        codec_(std::move(codec)),
        executor_(std::move(extension)),
        state_code_(std::move(state_code)) {}

  outcome::result<std::optional<ScheduledChange>> GrandpaImpl::pending_change(
      const Grandpa::Digest &digest) {
    OUTCOME_TRY(encoded_digest, codec_->encodeDigest(digest));

    runtime::SizeType ext_size = encoded_digest.size();
    // TODO (yuraz) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, encoded_digest);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "GrandpaApi_grandpa_pending_change", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return codec_->decodeScheduledChange(s);
  }

  outcome::result<std::optional<ForcedChange>> GrandpaImpl::forced_change(
      const Grandpa::Digest &digest) {
    OUTCOME_TRY(encoded_digest, codec_->encodeDigest(digest));

    runtime::SizeType ext_size = encoded_digest.size();
    // TODO (yuraz) PRE-98: after check for memory overflow is done, refactor it
    runtime::WasmPointer ptr = memory_->allocate(ext_size);
    memory_->storeBuffer(ptr, encoded_digest);

    wasm::LiteralList ll{wasm::Literal(ptr), wasm::Literal(ext_size)};

    OUTCOME_TRY(
        res,
        executor_.call(state_code_, "GrandpaApi_grandpa_forced_change", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return codec_->decodeForcedChange(s);
  }

  outcome::result<std::vector<WeightedAuthority>> GrandpaImpl::authorities() {
    wasm::LiteralList ll{wasm::Literal(0), wasm::Literal(0)};

    OUTCOME_TRY(
        res, executor_.call(state_code_, "GrandpaApi_grandpa_authorities", ll));

    uint32_t res_addr = static_cast<uint64_t>(res.geti64()) & 0xFFFFFFFFull;

    WasmMemoryStream s(memory_);
    OUTCOME_TRY(s.advance(res_addr));

    return codec_->decodeGrandpaAuthorities(s);
  }
}  // namespace kagome::runtime

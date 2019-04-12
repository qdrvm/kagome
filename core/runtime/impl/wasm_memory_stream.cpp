/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_memory_stream.hpp"

namespace kagome::runtime {

  WasmMemoryStream::WasmMemoryStream(std::shared_ptr<WasmMemory> memory)
      : memory_(std::move(memory)), current_ptr_{0} {}

  bool WasmMemoryStream::hasMore(uint64_t n) const {
    return current_ptr_ + n <= memory_->size();
  }

  std::optional<uint8_t> WasmMemoryStream::nextByte() {
    if (not hasMore(1)) {
      return std::nullopt;
    }

    return memory_->load8u(current_ptr_++);
  }

  outcome::result<void> WasmMemoryStream::advance(uint64_t dist) {
    if (not hasMore(dist)) {
      return AdvanceErrc::OUT_OF_BOUNDARIES;
    }
    current_ptr_ += dist;
    return outcome::success();
  }

}  // namespace kagome::runtime

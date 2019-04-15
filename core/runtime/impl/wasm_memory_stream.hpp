/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_WASM_MEMORY_STREAM_HPP_
#define KAGOME_CORE_RUNTIME_IMPL_WASM_MEMORY_STREAM_HPP_

#include "common/byte_stream.hpp"
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  class WasmMemoryStream : public common::ByteStream {
   public:
    explicit WasmMemoryStream(std::shared_ptr<WasmMemory> memory);

    ~WasmMemoryStream() override = default;

    bool hasMore(uint64_t n) const override;

    std::optional<uint8_t> nextByte() override;

    outcome::result<void> advance(uint64_t dist) override;

   private:
    std::shared_ptr<WasmMemory> memory_;
    WasmPointer current_ptr_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_WASM_MEMORY_STREAM_HPP_

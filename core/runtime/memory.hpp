/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

#include <optional>

#include "common/buffer.hpp"
#include "common/buffer_view.hpp"
#include "common/literals.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  using BytesOut = std::span<uint8_t>;

  inline constexpr size_t kInitialMemorySize = []() {
    using kagome::common::literals::operator""_MB;
    return 2_MB;
  }();

  // according to $3.1.2.1 in the Polkadot Host Spec
  // https://webassembly.github.io/spec/core/exec/runtime.html#memory-instances
  inline constexpr size_t kMemoryPageSize = []() {
    using kagome::common::literals::operator""_kB;
    return 64_kB;
  }();

  inline uint64_t sizeToPages(uint64_t size) {
    return (size + kMemoryPageSize - 1) / kMemoryPageSize;
  }

  /** The underlying memory can be accessed through unaligned pointers which
   * isn't well-behaved in C++. WebAssembly nonetheless expects it to behave
   * properly. Avoid emitting unaligned load/store by checking for alignment
   * explicitly, and performing memcpy if unaligned.
   *
   * The allocated memory tries to have the same alignment as the memory being
   * simulated.
   */
  class Memory {
   public:
    virtual ~Memory() = default;

    /**
     * @brief Return the size of the memory
     */
    virtual WasmSize size() const = 0;

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    virtual void resize(WasmSize new_size) = 0;

    virtual outcome::result<BytesOut> view(WasmPointer ptr,
                                           WasmSize size) const = 0;

    outcome::result<BytesOut> view(PtrSize ptr_size) const {
      return view(ptr_size.ptr, ptr_size.size);
    }

    outcome::result<BytesOut> view(WasmSpan span) const {
      return view(PtrSize{span});
    }

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then -1 is returned
     */
    virtual WasmPointer allocate(WasmSize size) = 0;

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    virtual void deallocate(WasmPointer ptr) = 0;

    common::BufferView loadN(WasmPointer ptr, WasmSize size) const {
      return view(ptr, size).value();
    }

    void storeBuffer(WasmPointer ptr, common::BufferView v) {
      memcpy(view(ptr, v.size()).value().data(), v.data(), v.size());
    }

    WasmSpan storeBuffer(common::BufferView v) {
      auto ptr = allocate(v.size());
      storeBuffer(ptr, v);
      return PtrSize{ptr, static_cast<WasmSize>(v.size())}.combine();
    }
  };
}  // namespace kagome::runtime

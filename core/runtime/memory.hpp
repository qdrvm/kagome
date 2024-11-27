/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "common/buffer_view.hpp"
#include "common/literals.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  using BytesOut = std::span<uint8_t>;

  // according to $3.1.2.1 in the Polkadot Host Spec
  // https://webassembly.github.io/spec/core/exec/runtime.html#memory-instances
  inline constexpr size_t kMemoryPageSize = []() {
    using kagome::common::literals::operator""_kB;
    return 64_kB;
  }();

  inline uint64_t sizeToPages(uint64_t size) {
    return (size + kMemoryPageSize - 1) / kMemoryPageSize;
  }

  class MemoryAllocator;

  /**
   * An interface for a particular WASM engine memory implementation
   */
  class MemoryHandle {
   public:
    virtual ~MemoryHandle() = default;

    virtual WasmSize size() const = 0;

    virtual std::optional<WasmSize> pagesMax() const = 0;

    virtual void resize(WasmSize new_size) = 0;

    virtual outcome::result<BytesOut> view(WasmPointer ptr,
                                           WasmSize size) const = 0;

    outcome::result<BytesOut> view(PtrSize ptr_size) const {
      return view(ptr_size.ptr, ptr_size.size);
    }

    outcome::result<BytesOut> view(WasmSpan span) const {
      return view(PtrSize{span});
    }
  };

  /**
   * A convenience wrapper around a memory handle and a memory allocator.
   *
   * Mind that the underlying memory can be accessed through unaligned pointers
   * which isn't well-behaved in C++. WebAssembly nonetheless expects it to
   * behave properly. Avoid emitting unaligned load/store by checking for
   * alignment explicitly, and performing memcpy if unaligned.
   *
   * The allocated memory tries to have the same alignment as the memory being
   * simulated.
   */
  class Memory final {
   public:
    Memory(std::shared_ptr<MemoryHandle> handle,
           std::unique_ptr<MemoryAllocator> allocator)
        : handle_{std::move(handle)}, allocator_{std::move(allocator)} {
      BOOST_ASSERT(handle_);
      BOOST_ASSERT(allocator_);
    }

    outcome::result<BytesOut> view(WasmPointer ptr, WasmSize size) const {
      return handle_->view(ptr, size);
    }

    outcome::result<BytesOut> view(PtrSize ptr_size) const {
      return handle_->view(ptr_size);
    }

    outcome::result<BytesOut> view(WasmSpan span) const {
      return handle_->view(span);
    }

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then -1 is returned
     */
    WasmPointer allocate(WasmSize size) {
      return allocator_->allocate(size);
    }

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    void deallocate(WasmPointer ptr) {
      return allocator_->deallocate(ptr);
    }

    common::BufferView loadN(WasmPointer ptr, WasmSize size) const {
      return handle_->view(ptr, size).value();
    }

    void storeBuffer(WasmPointer ptr, common::BufferView v) {
      if (v.empty()) {
        return;
      }
      memcpy(handle_->view(ptr, v.size()).value().data(), v.data(), v.size());
    }

    WasmSpan storeBuffer(common::BufferView v) {
      auto ptr = allocate(v.size());
      storeBuffer(ptr, v);
      return PtrSize{ptr, static_cast<WasmSize>(v.size())}.combine();
    }

    auto &memory() const {
      return handle_;
    }

   private:
    std::shared_ptr<MemoryHandle> handle_;
    std::unique_ptr<MemoryAllocator> allocator_;
  };
}  // namespace kagome::runtime

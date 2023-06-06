/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/memory_allocator.hpp"

#include "runtime/memory.hpp"

namespace kagome::runtime {

  static_assert(roundUpAlign(kDefaultHeapBase) == kDefaultHeapBase,
                "Heap base must be aligned");

  static_assert(kDefaultHeapBase < kInitialMemorySize,
                "Heap base must be in memory");

  MemoryAllocator::MemoryAllocator(MemoryHandle memory, WasmPointer heap_base)
      : memory_{std::move(memory)},
        offset_{roundUpAlign(heap_base)},
        logger_{log::createLogger("Allocator", "runtime")} {
    // Heap base (and offset in according) must be non-zero to prohibit
    // allocating memory at 0 in the future, as returning 0 from allocate method
    // means that wasm memory was exhausted
    BOOST_ASSERT(offset_ > 0);
    BOOST_ASSERT(memory_.getSize);
    BOOST_ASSERT(memory_.resize);
  }

  WasmPointer MemoryAllocator::allocate(WasmSize size, bool first_entry) {
    if (size == 0) {
      return 0;
    }

    const size_t sz = nextHighPowerOf2(
        roundUpAlign(size)
        + (first_entry ? roundUpAlign(sizeof(uint32_t)) : 0ull));

    const auto ptr = offset_;
    const auto new_offset = ptr + sz;  // align

    // Round up allocating chunk of memory
    size = sz;
    if (new_offset <= memory_.getSize()) {
      offset_ = new_offset;
      memory_.storeSz(ptr, size);
      SL_TRACE_FUNC_CALL(logger_, ptr, this, size);
      return ptr + roundUpAlign(sizeof(uint32_t));
    }

    if (first_entry) {
      auto &preallocates = available_[sz];
      if (!preallocates.empty()) {
        const auto ptr = preallocates.back();
        preallocates.pop_back();

        memory_.storeSz(ptr, sz);
        return ptr + roundUpAlign(sizeof(uint32_t));
      }
    }

    return growAlloc(size);
  }

  std::optional<WasmSize> MemoryAllocator::deallocate(WasmPointer ptr) {
    const auto sz = memory_.loadSz(ptr - roundUpAlign(sizeof(uint32_t)));
    available_[sz].push_back(ptr - roundUpAlign(sizeof(uint32_t)));
    return sz;
  }

  WasmPointer MemoryAllocator::growAlloc(WasmSize size) {
    // check that we do not exceed max memory size
    if (Memory::kMaxMemorySize - offset_ < size) {
      logger_->error(
          "Memory size exceeded when growing it on {} bytes, offset was 0x{:x}",
          size,
          offset_);
      return 0;
    }
    // try to increase memory size up to offset + size * 4 (we multiply by 4
    // to have more memory than currently needed to avoid resizing every time
    // when we exceed current memory)
    resize(offset_ + size);
    return allocate(size, false);
  }

  void MemoryAllocator::resize(WasmSize new_size) {
    memory_.resize(new_size);
  }

  /*
    Slow function with O(N) complexity. Can be used ONLY in tests.
  */
  std::optional<WasmSize> MemoryAllocator::getDeallocatedChunkSize(
      WasmPointer ptr) const {
    for (const auto &[sz, ptrs] : available_) {
      for (const auto &p : ptrs) {
        if (ptr == p) {
          return sz;
        }
      }
    }

    return std::nullopt;
  }

  std::optional<WasmSize> MemoryAllocator::getAllocatedChunkSize(
      WasmPointer ptr) const {
    return memory_.loadSz(ptr - roundUpAlign(sizeof(uint32_t)));
  }

  size_t MemoryAllocator::getDeallocatedChunksNum() const {
    size_t size = 0ull;
    for (const auto &[sz, ptrs] : available_) {
      size += ptrs.size();
    }

    return size;
  }

}  // namespace kagome::runtime

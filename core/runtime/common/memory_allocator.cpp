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

  WasmPointer MemoryAllocator::allocate(WasmSize size,
                                        bool search_in_deallocates) {
    if (size == 0) {
      return 0;
    }

    const size_t sz = nextHighPowerOf2(
        roundUpAlign(size)
        + (search_in_deallocates ? roundUpAlign(sizeof(uint32_t)) : 0ull));
    if (search_in_deallocates) {
      auto &preallocates = available_[sz];
      if (!preallocates.empty()) {
        const auto ptr = preallocates.top();
        preallocates.pop();

        memory_.storeSz(ptr, sz);
        return ptr + roundUpAlign(sizeof(uint32_t));
      }
    }

    const auto ptr = offset_;
    const auto new_offset = ptr + sz;  // align

    // Round up allocating chunk of memory
    size = sz;
    if (Memory::kMaxMemorySize - offset_ < size) {  // overflow
      logger_->error(
          "overflow occurred while trying to allocate {} bytes at offset "
          "0x{:x}",
          size,
          offset_);
      return 0;
    }

    if (new_offset <= memory_.getSize()) {
      offset_ = new_offset;
      memory_.storeSz(ptr, size);
      SL_TRACE_FUNC_CALL(logger_, ptr, this, size);
      return ptr + roundUpAlign(sizeof(uint32_t));
    }

    return growAlloc(size);
  }

  std::optional<WasmSize> MemoryAllocator::deallocate(WasmPointer ptr) {
    const auto sz = memory_.loadSz(ptr - roundUpAlign(sizeof(uint32_t)));
    available_[sz].push(ptr - roundUpAlign(sizeof(uint32_t)));
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

  std::optional<WasmSize> MemoryAllocator::getDeallocatedChunkSize(
      WasmPointer ptr) const {
    return std::nullopt;
  }

  std::optional<WasmSize> MemoryAllocator::getAllocatedChunkSize(
      WasmPointer ptr) const {
    return std::nullopt;
  }

  size_t MemoryAllocator::getAllocatedChunksNum() const {
    return 0ull;  // allocated_.size();
  }

  size_t MemoryAllocator::getDeallocatedChunksNum() const {
    return 0ull;  // deallocated_.size();
  }

}  // namespace kagome::runtime

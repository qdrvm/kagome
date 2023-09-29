/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/memory_allocator.hpp"

#include "log/formatters/ref_and_ptr.hpp"
#include "log/trace_macros.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {

  static_assert(roundUpAlign(kDefaultHeapBase) == kDefaultHeapBase,
                "Heap base must be aligned");

  static_assert(kDefaultHeapBase < kInitialMemorySize,
                "Heap base must be in memory");

  MemoryAllocator::MemoryAllocator(MemoryHandle memory,
                                   const MemoryConfig &config)
      : memory_{std::move(memory)},
        offset_{roundUpAlign(config.heap_base)},
        max_memory_pages_num_{config.limits.max_memory_pages_num.value_or(
            std::numeric_limits<WasmSize>::max())},
        logger_{log::createLogger("Allocator", "runtime")} {
    // Heap base (and offset in according) must be non-zero to prohibit
    // allocating memory at 0 in the future, as returning 0 from allocate method
    // means that wasm memory was exhausted
    BOOST_ASSERT(offset_ > 0);
    BOOST_ASSERT(max_memory_pages_num_ > 0);
    BOOST_ASSERT(memory_.getSize);
    BOOST_ASSERT(memory_.resize);
  }

  WasmPointer MemoryAllocator::allocate(const WasmSize size) {
    if (size == 0) {
      return 0;
    }

    const size_t chunk_size =
        math::nextHighPowerOf2(roundUpAlign(size) + AllocationHeaderSz);

    const auto ptr = offset_;
    const auto new_offset = ptr + chunk_size;  // align

    // Round up allocating chunk of memory
    if (new_offset <= memory_.getSize()) {
      offset_ = new_offset;
      AllocationHeader{
          .chunk_sz = (uint32_t)chunk_size,
          .allocation_sz = roundUpAlign(size),
      }
          .serialize(ptr, memory_);
      SL_TRACE_FUNC_CALL(logger_, ptr, static_cast<const void *>(this), size);
      return ptr + AllocationHeaderSz;
    }

    auto &preallocates = available_[chunk_size];
    if (!preallocates.empty()) {
      const auto ptr = preallocates.back();
      preallocates.pop_back();

      AllocationHeader{
          .chunk_sz = (uint32_t)chunk_size,
          .allocation_sz = roundUpAlign(size),
      }
          .serialize(ptr, memory_);
      return ptr + AllocationHeaderSz;
    }

    return growAlloc(chunk_size, size);
  }

  std::optional<WasmSize> MemoryAllocator::deallocate(WasmPointer ptr) {
    AllocationHeader header{
        .chunk_sz = 0,
        .allocation_sz = 0,
    };
    header.deserialize(ptr - AllocationHeaderSz, memory_);
    BOOST_ASSERT(math::isPowerOf2(header.chunk_sz));

    available_[header.chunk_sz].push_back(ptr - AllocationHeaderSz);
    BOOST_ASSERT(!available_.empty());
    return header.allocation_sz;
  }

  WasmPointer MemoryAllocator::growAlloc(size_t chunk_sz,
                                         WasmSize allocation_sz) {
    // check that we do not exceed max memory size
    auto new_pages_num =
        (chunk_sz + offset_ + kMemoryPageSize - 1) / kMemoryPageSize;
    if (new_pages_num > max_memory_pages_num_) {
      logger_->error(
          "Memory size exceeded when growing it on {} bytes, offset was 0x{:x}",
          chunk_sz,
          offset_);
      return 0;
    }
    auto new_size = offset_ + chunk_sz;
    if (new_size > std::numeric_limits<WasmSize>::max()) {
      return 0;
    }

    resize(new_size);
    BOOST_ASSERT(memory_.getSize() >= new_size);
    return allocate(allocation_sz);
  }

  void MemoryAllocator::resize(WasmSize new_size) {
    memory_.resize(new_size);
  }

  std::optional<WasmSize> MemoryAllocator::getDeallocatedChunkSize(
      WasmPointer ptr) const {
    for (const auto &[chunk_size, ptrs] : available_) {
      for (const auto &p : ptrs) {
        if (ptr == p) {
          return chunk_size;
        }
      }
    }

    return std::nullopt;
  }

  std::optional<WasmSize> MemoryAllocator::getAllocatedChunkSize(
      WasmPointer ptr) const {
    AllocationHeader header{
        .chunk_sz = 0,
        .allocation_sz = 0,
    };
    header.deserialize(ptr - AllocationHeaderSz, memory_);
    BOOST_ASSERT(math::isPowerOf2(header.chunk_sz));

    return header.allocation_sz;
  }

  size_t MemoryAllocator::getDeallocatedChunksNum() const {
    size_t size = 0ull;
    for (const auto &[_, ptrs] : available_) {
      size += ptrs.size();
    }

    return size;
  }

}  // namespace kagome::runtime

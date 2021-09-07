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

  MemoryAllocator::MemoryAllocator(MemoryHandle memory,
                                   size_t size,
                                   WasmPointer heap_base)
      : memory_{std::move(memory)},
        offset_{heap_base},
        size_{size},
        logger_{log::createLogger("Allocator", "runtime")} {
    // Heap base (and offset in according) must be non-zero to prohibit
    // allocating memory at 0 in the future, as returning 0 from allocate method
    // means that wasm memory was exhausted
    BOOST_ASSERT(offset_ > 0);
    BOOST_ASSERT(memory_.getSize);
    BOOST_ASSERT(memory_.resize);

    size_ = std::max(size_, offset_);
    BOOST_ASSERT(offset_ <= Memory::kMaxMemorySize - size_);
  }

  void MemoryAllocator::reset() {
    allocated_.clear();
    deallocated_.clear();
  }

  WasmPointer MemoryAllocator::allocate(WasmSize size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = roundUpAlign(ptr + size);  // align

    // Round up allocating chunk of memory
    size = new_offset - ptr;

    BOOST_ASSERT(allocated_.find(ptr) == allocated_.end());
    if (Memory::kMaxMemorySize - offset_ < size) {  // overflow
      logger_->error(
          "overflow occurred while trying to allocate {} bytes at offset "
          "0x{:x}",
          size,
          offset_);
      return 0;
    }
    if (new_offset <= size_) {
      offset_ = new_offset;
      allocated_[ptr] = size;
      SL_TRACE_FUNC_CALL(logger_, ptr, this, size);
      return ptr;
    }

    auto res = freealloc(size);
    SL_TRACE_FUNC_CALL(logger_, res, this, size);
    return res;
  }

  std::optional<WasmSize> MemoryAllocator::deallocate(WasmPointer ptr) {
    auto a_it = allocated_.find(ptr);
    if (a_it == allocated_.end()) {
      return std::nullopt;
    }

    auto a_node = allocated_.extract(a_it);
    auto size = a_node.mapped();
    auto [d_it, is_emplaced] = deallocated_.emplace(ptr, size);
    BOOST_ASSERT(is_emplaced);

    // Combine with next chunk if it adjacent
    while (true) {
      auto node = deallocated_.extract(ptr + size);
      if (not node) break;
      d_it->second += node.mapped();
    }

    // Combine with previous chunk if it adjacent
    while (deallocated_.begin() != d_it) {
      auto d_it_prev = std::prev(d_it);
      if (d_it_prev->first + d_it_prev->second != d_it->first) {
        break;
      }
      d_it_prev->second += d_it->second;
      deallocated_.erase(d_it);
      d_it = d_it_prev;
    }

    auto d_it_next = std::next(d_it);
    if (d_it_next == deallocated_.end()) {
      if (d_it->first + d_it->second == offset_) {
        offset_ = d_it->first;
        deallocated_.erase(d_it);
      }
    }

    SL_TRACE_FUNC_CALL(logger_, size, this, ptr);
    return size;
  }

  WasmPointer MemoryAllocator::freealloc(WasmSize size) {
    if (size == 0) {
      return 0;
    }

    // Round up size of allocating memory chunk
    size = roundUpAlign(size);

    auto min_chunk_size = std::numeric_limits<WasmPointer>::max();
    WasmPointer ptr = 0;
    for (const auto &[chunk_ptr, chunk_size] : deallocated_) {
      BOOST_ASSERT(chunk_size > 0);
      if (chunk_size >= size and chunk_size < min_chunk_size) {
        min_chunk_size = chunk_size;
        ptr = chunk_ptr;
        if (min_chunk_size == size) {
          break;
        }
      }
    }
    if (ptr == 0) {
      // if did not find available space among deallocated memory chunks,
      // then grow memory and allocate in new space
      return growAlloc(size);
    }

    const auto node = deallocated_.extract(ptr);
    BOOST_ASSERT(node);

    auto old_size = node.mapped();
    if (old_size > size) {
      auto new_ptr = ptr + size;
      auto new_size = old_size - size;
      BOOST_ASSERT(new_size > 0);

      deallocated_[new_ptr] = new_size;
    }

    allocated_[ptr] = size;

    return ptr;
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
    if ((Memory::kMaxMemorySize - offset_) / 4 > size) {
      resize(offset_ + size * 4);
    } else {
      // if we can't increase by size * 4 then increase memory size by
      // provided size
      resize(offset_ + size);
    }
    return allocate(size);
  }

  void MemoryAllocator::resize(WasmSize new_size) {
    /**
     * We use this condition to avoid deallocated_ pointers fixup
     */
    BOOST_ASSERT(offset_ <= Memory::kMaxMemorySize - new_size);
    if (new_size >= size_) {
      size_ = new_size;
      memory_.resize(new_size);
    }
  }

  std::optional<WasmSize> MemoryAllocator::getDeallocatedChunkSize(
      WasmPointer ptr) const {
    auto it = deallocated_.find(ptr);
    return it != deallocated_.cend() ? std::make_optional(it->second)
                                     : std::nullopt;
  }

  std::optional<WasmSize> MemoryAllocator::getAllocatedChunkSize(
      WasmPointer ptr) const {
    auto it = allocated_.find(ptr);
    return it != allocated_.cend() ? std::make_optional(it->second)
                                   : std::nullopt;
  }

  size_t MemoryAllocator::getAllocatedChunksNum() const {
    return allocated_.size();
  }

  size_t MemoryAllocator::getDeallocatedChunksNum() const {
    return deallocated_.size();
  }

}  // namespace kagome::runtime

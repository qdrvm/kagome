/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/memory_allocator.hpp"

#include <boost/endian/conversion.hpp>

#include "runtime/memory.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/polkadot-sdk/blob/polkadot-v1.7.0/substrate/client/allocator/src/lib.rs#L39
  constexpr auto kMaxPages = (uint64_t{4} << 30) / kMemoryPageSize;

  static_assert(roundUpAlign(kDefaultHeapBase) == kDefaultHeapBase,
                "Heap base must be aligned");

  static_assert(kDefaultHeapBase < kInitialMemorySize,
                "Heap base must be in memory");

  inline uint64_t read_u64(const Memory &memory, WasmPointer ptr) {
    return boost::endian::load_little_u64(
        memory.view(ptr, sizeof(uint64_t)).value().data());
  }

  inline void write_u64(const Memory &memory, WasmPointer ptr, uint64_t v) {
    boost::endian::store_little_u64(
        memory.view(ptr, sizeof(uint64_t)).value().data(), v);
  }

  MemoryAllocator::MemoryAllocator(Memory &memory, const MemoryConfig &config)
      : memory_{memory},
        offset_{roundUpAlign(config.heap_base)},
        max_memory_pages_num_{
            config.limits.max_memory_pages_num.value_or(kMaxPages)} {
    BOOST_ASSERT(max_memory_pages_num_ > 0);
  }

  WasmPointer MemoryAllocator::allocate(WasmSize size) {
    if (size > kMaxAllocate) {
      throw std::runtime_error{"RequestedAllocationTooLarge"};
    }
    size = std::max(size, kMinAllocate);
    size = math::nextHighPowerOf2(size);
    uint32_t order = std::countr_zero(size) - std::countr_zero(kMinAllocate);
    uint32_t head_ptr;
    if (auto &list = free_lists_.at(order)) {
      head_ptr = *list;
      if (*list + sizeof(Header) + size > memory_.size()) {
        throw std::runtime_error{"Invalid header pointer detected"};
      }
      list = readFree(*list);
    } else {
      head_ptr = offset_;
      auto next_offset = uint64_t{offset_} + sizeof(Header) + size;
      if (next_offset > memory_.size()) {
        auto pages = sizeToPages(next_offset);
        if (pages > max_memory_pages_num_) {
          throw std::runtime_error{"AllocatorOutOfSpace"};
        }
        pages = std::max(pages, 2 * sizeToPages(memory_.size()));
        pages = std::min<uint64_t>(pages, max_memory_pages_num_);
        memory_.resize(pages * kMemoryPageSize);
      }
      offset_ = next_offset;
    }
    write_u64(memory_, head_ptr, kOccupied | order);
    return head_ptr + sizeof(Header);
  }

  void MemoryAllocator::deallocate(WasmPointer ptr) {
    if (ptr < sizeof(Header)) {
      throw std::runtime_error{"Invalid pointer for deallocation"};
    }
    auto head_ptr = ptr - sizeof(Header);
    auto order = readOccupied(head_ptr);
    auto &list = free_lists_.at(order);
    auto prev = list.value_or(kNil);
    list = head_ptr;
    write_u64(memory_, head_ptr, prev);
  }

  uint32_t MemoryAllocator::readOccupied(WasmPointer head_ptr) const {
    auto head = read_u64(memory_, head_ptr);
    uint32_t order = head;
    if (order >= kOrders) {
      throw std::runtime_error{"invalid order"};
    }
    if ((head & kOccupied) == 0) {
      throw std::runtime_error{"the allocation points to an empty header"};
    }
    return order;
  }

  std::optional<uint32_t> MemoryAllocator::readFree(
      WasmPointer head_ptr) const {
    auto head = read_u64(memory_, head_ptr);
    if ((head & kOccupied) != 0) {
      throw std::runtime_error{"free list points to a occupied header"};
    }
    uint32_t prev = head;
    if (prev == kNil) {
      return std::nullopt;
    }
    return prev;
  }

  std::optional<WasmSize> MemoryAllocator::getAllocatedChunkSize(
      WasmPointer ptr) const {
    return kMinAllocate << readOccupied(ptr - sizeof(Header));
  }

  size_t MemoryAllocator::getDeallocatedChunksNum() const {
    size_t size = 0ull;
    for (auto list : free_lists_) {
      while (list) {
        ++size;
        list = readFree(*list);
      }
    }

    return size;
  }

}  // namespace kagome::runtime

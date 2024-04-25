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

  constexpr auto kPoisoned{"the allocator has been poisoned"};

  static uint64_t read_u64(const MemoryHandle &memory, WasmPointer ptr) {
    return boost::endian::load_little_u64(
        memory.view(ptr, sizeof(uint64_t)).value().data());
  }

  static void write_u64(const MemoryHandle &memory,
                        WasmPointer ptr,
                        uint64_t v) {
    boost::endian::store_little_u64(
        memory.view(ptr, sizeof(uint64_t)).value().data(), v);
  }

  MemoryAllocatorImpl::MemoryAllocatorImpl(std::shared_ptr<MemoryHandle> memory,
                                           const MemoryConfig &config)
      : memory_{memory},
        offset_{roundUpAlign(config.heap_base)},
        max_memory_pages_num_{memory_->pagesMax().value_or(kMaxPages)} {
    BOOST_ASSERT(max_memory_pages_num_ > 0);
  }

  WasmPointer MemoryAllocatorImpl::allocate(WasmSize size) {
    if (poisoned_) {
      throw std::runtime_error{kPoisoned};
    }
    poisoned_ = true;
    if (size > kMaxAllocate) {
      throw std::runtime_error{"RequestedAllocationTooLarge"};
    }
    size = std::max(size, kMinAllocate);
    size = math::nextHighPowerOf2(size);
    uint32_t order = std::countr_zero(size) - std::countr_zero(kMinAllocate);
    uint32_t head_ptr;
    if (auto &list = free_lists_.at(order)) {
      head_ptr = *list;
      if (*list + sizeof(Header) + size > memory_->size()) {
        throw std::runtime_error{"Header pointer out of memory bounds"};
      }
      list = readFree(*list);
    } else {
      head_ptr = offset_;
      auto next_offset = uint64_t{offset_} + sizeof(Header) + size;
      if (next_offset > memory_->size()) {
        auto pages = sizeToPages(next_offset);
        if (pages > max_memory_pages_num_) {
          throw std::runtime_error{
              "Memory resize failed, because maximum number of pages is reached."};
        }
        pages = std::max(pages, 2 * sizeToPages(memory_->size()));
        pages = std::min<uint64_t>(pages, max_memory_pages_num_);
        memory_->resize(pages * kMemoryPageSize);
      }
      offset_ = next_offset;
    }
    write_u64(*memory_, head_ptr, kOccupied | order);
    poisoned_ = false;
    return head_ptr + sizeof(Header);
  }

  void MemoryAllocatorImpl::deallocate(WasmPointer ptr) {
    if (poisoned_) {
      throw std::runtime_error{kPoisoned};
    }
    poisoned_ = true;
    if (ptr < sizeof(Header)) {
      throw std::runtime_error{"Invalid pointer for deallocation"};
    }
    auto head_ptr = ptr - sizeof(Header);
    auto order = readOccupied(head_ptr);
    auto &list = free_lists_.at(order);
    auto prev = list.value_or(kNil);
    list = head_ptr;
    write_u64(*memory_, head_ptr, prev);
    poisoned_ = false;
  }

  uint32_t MemoryAllocatorImpl::readOccupied(WasmPointer head_ptr) const {
    auto head = read_u64(*memory_, head_ptr);
    uint32_t order = head;
    if (order >= kOrders) {
      throw std::runtime_error{"order exceed the total number of orders"};
    }
    if ((head & kOccupied) == 0) {
      throw std::runtime_error{"the allocation points to an empty header"};
    }
    return order;
  }

  std::optional<uint32_t> MemoryAllocatorImpl::readFree(
      WasmPointer head_ptr) const {
    auto head = read_u64(*memory_, head_ptr);
    if ((head & kOccupied) != 0) {
      throw std::runtime_error{"free list points to a occupied header"};
    }
    uint32_t prev = head;
    if (prev == kNil) {
      return std::nullopt;
    }
    return prev;
  }

  std::optional<WasmSize> MemoryAllocatorImpl::getAllocatedChunkSize(
      WasmPointer ptr) const {
    return kMinAllocate << readOccupied(ptr - sizeof(Header));
  }

  size_t MemoryAllocatorImpl::getDeallocatedChunksNum() const {
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

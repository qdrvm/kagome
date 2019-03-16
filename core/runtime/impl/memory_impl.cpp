/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/memory_impl.hpp"

namespace kagome::runtime {

  MemoryImpl::MemoryImpl() {
    offset_ = 0;
  }

  MemoryImpl::MemoryImpl(SizeType size) : MemoryImpl() {
    memory_.resize(size);
  }

  void MemoryImpl::resize(uint32_t newSize) {
    // Ensure the smallest allocation is large enough that most allocators
    // will provide page-aligned storage. This hopefully allows the
    // interpreter's MemoryImpl to be as aligned as the MemoryImpl being
    // simulated, ensuring that the performance doesn't needlessly degrade.
    //
    // The code is optimistic this will work until WG21's p0035r0 happens.
    const size_t minSize = 1 << 12;
    size_t oldSize = memory_.size();
    memory_.resize(std::max(minSize, static_cast<size_t>(newSize)));
    if (newSize < oldSize && newSize < minSize) {
      std::memset(&memory_[newSize], 0, minSize - newSize);
    }
  }

  std::optional<Memory::AddressType> MemoryImpl::allocate(SizeType size) {
    if (size == 0) {
      return 0;
    }
    if (size > memory_.size()) {
      return {};
    }
    const auto ptr = offset_;
    const auto new_offset = ptr + size;

    if (new_offset <= memory_.size()) {
      offset_ = new_offset;
      allocated[ptr] = size;
      return ptr;
    }

    return freealloc(size);
  }

  std::optional<Memory::SizeType> MemoryImpl::deallocate(AddressType ptr) {
    const auto &it = allocated.find(ptr);
    if (it == allocated.end()) {
      return std::nullopt;
    }
    const auto size = it->second;

    allocated.erase(ptr);
    deallocated[ptr] = size;

    return size;
  }

  std::optional<Memory::AddressType> MemoryImpl::freealloc(SizeType size) {
    return findContaining(size) | [&](const auto ptr) {
      deallocated.erase(ptr);
      allocated[ptr] = size;

      return std::make_optional(ptr);
    };
  }

  std::optional<Memory::AddressType> MemoryImpl::findContaining(SizeType size) {
    auto min_value = std::numeric_limits<AddressType>::max();
    std::optional<AddressType> min_key = std::nullopt;
    for (const auto &[key, value] : deallocated) {
      if (value < min_value and value >= size) {
        min_value = value;
        min_key = key;
      }
    }
    return min_key;
  }

  int8_t MemoryImpl::load8s(Memory::AddressType addr) {
    return get<int8_t>(addr);
  }
  uint8_t MemoryImpl::load8u(Memory::AddressType addr) {
    return get<uint8_t>(addr);
  }
  int16_t MemoryImpl::load16s(Memory::AddressType addr) {
    return get<int16_t>(addr);
  }
  uint16_t MemoryImpl::load16u(Memory::AddressType addr) {
    return get<uint16_t>(addr);
  }
  int32_t MemoryImpl::load32s(Memory::AddressType addr) {
    return get<int32_t>(addr);
  }
  uint32_t MemoryImpl::load32u(Memory::AddressType addr) {
    return get<uint32_t>(addr);
  }
  int64_t MemoryImpl::load64s(Memory::AddressType addr) {
    return get<int64_t>(addr);
  }
  uint64_t MemoryImpl::load64u(Memory::AddressType addr) {
    return get<uint64_t>(addr);
  }
  std::array<uint8_t, 16> MemoryImpl::load128(Memory::AddressType addr) {
    return get<std::array<uint8_t, 16>>(addr);
  }

  void MemoryImpl::store8(Memory::AddressType addr, int8_t value) {
    set<int8_t>(addr, value);
  }
  void MemoryImpl::store16(Memory::AddressType addr, int16_t value) {
    set<int16_t>(addr, value);
  }
  void MemoryImpl::store32(Memory::AddressType addr, int32_t value) {
    set<int32_t>(addr, value);
  }
  void MemoryImpl::store64(Memory::AddressType addr, int64_t value) {
    set<int64_t>(addr, value);
  }
  void MemoryImpl::store128(Memory::AddressType addr,
                            const std::array<uint8_t, 16> &value) {
    set<std::array<uint8_t, 16>>(addr, value);
  }

}  // namespace kagome::runtime

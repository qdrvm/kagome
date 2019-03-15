/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/memory.hpp"

namespace kagome::extensions {

  Memory::Memory() {
    offset_ = 0;
  }

  Memory::Memory(size_t size) : Memory() {
    memory_.resize(size);
  }

  void Memory::resize(size_t newSize) {
    // Ensure the smallest allocation is large enough that most allocators
    // will provide page-aligned storage. This hopefully allows the
    // interpreter's memory to be as aligned as the memory being simulated,
    // ensuring that the performance doesn't needlessly degrade.
    //
    // The code is optimistic this will work until WG21's p0035r0 happens.
    const size_t minSize = 1 << 12;
    size_t oldSize = memory_.size();
    memory_.resize(std::max(minSize, newSize));
    if (newSize < oldSize && newSize < minSize) {
      std::memset(&memory_[newSize], 0, minSize - newSize);
    }
  }

  std::optional<Memory::Address> Memory::allocate(size_t size) {
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

  std::optional<size_t> Memory::deallocate(Memory::Address ptr) {
    const auto &it = allocated.find(ptr);
    if (it == allocated.end()) {
      return {};
    }
    const auto size = it->second;

    allocated.erase(ptr);
    deallocated[ptr] = size;

    return size;
  }

  std::optional<Memory::Address> Memory::freealloc(size_t size) {
    return findContaining(size) | [&](const auto ptr) {
      deallocated.erase(ptr);
      allocated[ptr] = size;

      return std::make_optional(ptr);
    };
  }

  std::optional<Memory::Address> Memory::findContaining(size_t size) {
    size_t min_value = std::numeric_limits<size_t>::max();
    std::optional<Address> min_key = std::nullopt;
    for (const auto &[key, value] : deallocated) {
      if (value < min_value and value >= size) {
        min_value = value;
        min_key = key;
      }
    }
    return min_key;
  }

}  // namespace kagome::extensions

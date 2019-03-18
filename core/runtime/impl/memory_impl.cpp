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
    resize(size);
  }

  void MemoryImpl::resize(uint32_t newSize) {
    // Ensure the smallest allocation is large enough that most allocators
    // will provide page-aligned storage. This hopefully allows the
    // interpreter's memory to be as aligned as the MemoryImpl being
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

  Memory::WasmPointer MemoryImpl::allocate(SizeType size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = ptr + size;

    if (new_offset < ptr) {  // overflow
      return -1;
    }
    if (new_offset <= memory_.size()) {
      offset_ = new_offset;
      allocated[ptr] = size;
      return ptr;
    }

    return freealloc(size);
  }

  std::optional<Memory::SizeType> MemoryImpl::deallocate(WasmPointer ptr) {
    const auto &it = allocated.find(ptr);
    if (it == allocated.end()) {
      return std::nullopt;
    }
    const auto size = it->second;

    allocated.erase(ptr);
    deallocated[ptr] = size;

    return size;
  }

  Memory::WasmPointer MemoryImpl::freealloc(SizeType size) {
    auto ptr = findContaining(size);
    if (ptr == -1) {
      // if did not find available space among deallocated memory chunks, then
      // grow memory and allocate in new space
      return growAlloc(size);
    }
    deallocated.erase(ptr);
    allocated[ptr] = size;
    return ptr;
  }

  Memory::WasmPointer MemoryImpl::findContaining(SizeType size) {
    auto min_value = std::numeric_limits<WasmPointer>::max();
    WasmPointer min_key = -1;
    for (const auto &[key, value] : deallocated) {
      if (value < min_value and value >= size) {
        min_value = value;
        min_key = key;
      }
    }
    return min_key;
  }

  Memory::WasmPointer MemoryImpl::growAlloc(SizeType size) {
    // check that we do not exceed max memory size
    if (offset_ > kMaxMemorySize - size) {
      return -1;
    }
    // try to increase memory size up to offset + size * 4 (we multiply by 4 to
    // have more memory than currently needed to avoid resizing every time when
    // we exceed current memory)
    if (offset_ < kMaxMemorySize - size * 4) {
      memory_.resize(offset_ + size * 4);
    } else {
      // if we can't increase by size * 4 then increase memory size by provided
      // size
      memory_.resize(offset_ + size);
    }
    return allocate(size);
  }

  int8_t MemoryImpl::load8s(Memory::WasmPointer addr) const {
    return get<int8_t>(addr);
  }
  uint8_t MemoryImpl::load8u(Memory::WasmPointer addr) const {
    return get<uint8_t>(addr);
  }
  int16_t MemoryImpl::load16s(Memory::WasmPointer addr) const {
    return get<int16_t>(addr);
  }
  uint16_t MemoryImpl::load16u(Memory::WasmPointer addr) const {
    return get<uint16_t>(addr);
  }
  int32_t MemoryImpl::load32s(Memory::WasmPointer addr) const {
    return get<int32_t>(addr);
  }
  uint32_t MemoryImpl::load32u(Memory::WasmPointer addr) const {
    return get<uint32_t>(addr);
  }
  int64_t MemoryImpl::load64s(Memory::WasmPointer addr) const {
    return get<int64_t>(addr);
  }
  uint64_t MemoryImpl::load64u(Memory::WasmPointer addr) const {
    return get<uint64_t>(addr);
  }
  std::array<uint8_t, 16> MemoryImpl::load128(Memory::WasmPointer addr) const {
    return get<std::array<uint8_t, 16>>(addr);
  }

  void MemoryImpl::store8(Memory::WasmPointer addr, int8_t value) {
    set<int8_t>(addr, value);
  }
  void MemoryImpl::store16(Memory::WasmPointer addr, int16_t value) {
    set<int16_t>(addr, value);
  }
  void MemoryImpl::store32(Memory::WasmPointer addr, int32_t value) {
    set<int32_t>(addr, value);
  }
  void MemoryImpl::store64(Memory::WasmPointer addr, int64_t value) {
    set<int64_t>(addr, value);
  }
  void MemoryImpl::store128(Memory::WasmPointer addr,
                            const std::array<uint8_t, 16> &value) {
    set<std::array<uint8_t, 16>>(addr, value);
  }

}  // namespace kagome::runtime

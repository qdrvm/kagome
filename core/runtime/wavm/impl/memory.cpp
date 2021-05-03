/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "memory.hpp"

#include "common/literals.hpp"

namespace kagome::runtime::wavm {

  using namespace kagome::common::literals;

  inline const size_t kInitialMemorySize = 2_MB;
  inline const size_t kDefaultHeapBase = 1_MB;

  Memory::Memory(WAVM::Runtime::Memory *memory)
      : memory_(memory),
        offset_{1},
        heap_base_{kDefaultHeapBase},
        logger_{log::createLogger("WavmMemory")},
        size_{kInitialMemorySize} {
    BOOST_ASSERT(memory_);
    BOOST_ASSERT(heap_base_ > 0);
    BOOST_ASSERT(heap_base_ == roundUpAlign(heap_base_));

    size_ = std::max(size_, offset_);
    resize(size_);
  }

  void Memory::setHeapBase(WasmSize heap_base) {
    BOOST_ASSERT(heap_base_ > 0);
    BOOST_ASSERT(heap_base_ == roundUpAlign(heap_base_));
    heap_base_ = heap_base;
  }

  void Memory::setUnderlyingMemory(WAVM::Runtime::Memory *memory) {
    BOOST_ASSERT(memory != nullptr);
    memory_ = memory;
    reset();
  }

  void Memory::reset() {
    offset_ = heap_base_;
    allocated_.clear();
    deallocated_.clear();
    if (size_ < offset_) {
      size_ = offset_;
      resize(size_);
    }
    SL_TRACE(logger_, "Memory reset; memory ptr: {}", fmt::ptr(memory_));
  }

  WasmPointer Memory::allocate(WasmSize size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = roundUpAlign(ptr + size);  // align

    // BOOST_ASSERT(allocated_.find(ptr) == allocated_.end());
    if (new_offset < static_cast<uint32_t>(ptr)) {  // overflow
      logger_->error(
          "overflow occurred while trying to allocate {} bytes at offset 0x{:x}",
          size,
          offset_);
      return 0;
    }
    if (new_offset <= this->size()) {
      offset_ = new_offset;
      allocated_[ptr] = size;
      return ptr;
    }

    return freealloc(size);
  }

  boost::optional<WasmSize> Memory::deallocate(WasmPointer ptr) {
    const auto it = allocated_.find(ptr);
    if (it == allocated_.end()) {
      return boost::none;
    }
    const auto size = it->second;

    allocated_.erase(ptr);
    deallocated_[ptr] = size;
    return size;
  }

  WasmPointer Memory::freealloc(WasmSize size) {
    auto ptr = findContaining(size);
    if (ptr == 0) {
      // if did not find available space among deallocated memory chunks, then
      // grow memory and allocate in new space
      return growAlloc(size);
    }
    allocated_[ptr] = deallocated_[ptr];
    deallocated_.erase(ptr);
    return ptr;
  }

  WasmPointer Memory::findContaining(WasmSize size) {
    auto min_value = std::numeric_limits<WasmPointer>::max();
    WasmPointer min_key = 0;
    for (const auto &[key, value] : deallocated_) {
      if (min_value <= 0) {
        return 0;
      }
      if (value < static_cast<uint32_t>(min_value) and value >= size) {
        min_value = value;
        min_key = key;
      }
    }
    return min_key;
  }

  WasmPointer Memory::growAlloc(WasmSize size) {
    // check that we do not exceed max memory size
    if (static_cast<uint32_t>(offset_) > kMaxMemorySize - size) {
      logger_->error(
          "Memory size exceeded when growing it on {} bytes, offset was 0x{:x}",
          size,
          offset_);
      return 0;
    }
    // try to increase memory size up to offset + size * 4 (we multiply by 4
    // to have more memory than currently needed to avoid resizing every time
    // when we exceed current memory)
    if (static_cast<uint32_t>(offset_) < kMaxMemorySize - size * 4) {
      resize(offset_ + size * 4);
    } else {
      // if we can't increase by size * 4 then increase memory size by
      // provided size
      resize(offset_ + size);
    }
    return allocate(size);
  }

  int8_t Memory::load8s(WasmPointer addr) const {
    return load<int8_t>(addr);
  }
  uint8_t Memory::load8u(WasmPointer addr) const {
    return load<uint8_t>(addr);
  }
  int16_t Memory::load16s(WasmPointer addr) const {
    return load<int16_t>(addr);
  }
  uint16_t Memory::load16u(WasmPointer addr) const {
    return load<uint16_t>(addr);
  }
  int32_t Memory::load32s(WasmPointer addr) const {
    return load<int32_t>(addr);
  }
  uint32_t Memory::load32u(WasmPointer addr) const {
    return load<uint32_t>(addr);
  }
  int64_t Memory::load64s(WasmPointer addr) const {
    return load<int64_t>(addr);
  }
  uint64_t Memory::load64u(WasmPointer addr) const {
    return load<uint64_t>(addr);
  }
  std::array<uint8_t, 16> Memory::load128(WasmPointer addr) const {
    return load<std::array<uint8_t, 16>>(addr);
  }

  common::Buffer Memory::loadN(kagome::runtime::WasmPointer addr,
                               kagome::runtime::WasmSize n) const {
    SL_TRACE(logger_, "loadN {} {}", addr, n);
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    common::Buffer res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.putUint8(load<uint8_t>(i));
    }
    return res;
  }

  std::string Memory::loadStr(kagome::runtime::WasmPointer addr,
                              kagome::runtime::WasmSize n) const {
    SL_TRACE(logger_, "loadStr {} {}", addr, n);
    std::string res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.push_back(load<uint8_t>(i));
    }
    return res;
  }

  void Memory::store8(WasmPointer addr, int8_t value) {
    store<int8_t>(addr, value);
  }
  void Memory::store16(WasmPointer addr, int16_t value) {
    store<int16_t>(addr, value);
  }
  void Memory::store32(WasmPointer addr, int32_t value) {
    store<int32_t>(addr, value);
  }
  void Memory::store64(WasmPointer addr, int64_t value) {
    store<int64_t>(addr, value);
  }
  void Memory::store128(WasmPointer addr,
                        const std::array<uint8_t, 16> &value) {
    store<std::array<uint8_t, 16>>(addr, value);
  }
  void Memory::storeBuffer(kagome::runtime::WasmPointer addr,
                           gsl::span<const uint8_t> value) {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    // boundaries, 04.04.2019
    SL_TRACE(logger_, "storeBuffer {} {}", addr, value.size());
    for (size_t i = addr, j = 0; i < addr + static_cast<size_t>(value.size());
         i++, j++) {
      store8(i, value[j]);
    }
  }

  WasmSpan Memory::storeBuffer(gsl::span<const uint8_t> value) {
    auto wasm_pointer = allocate(value.size());
    SL_TRACE(logger_, "storeBuffer (alloc) {} {}", wasm_pointer, value.size());
    if (wasm_pointer == 0) {
      return 0;
    }
    storeBuffer(wasm_pointer, value);
    return WasmResult(wasm_pointer, value.size()).combine();
  }
}  // namespace kagome::runtime::wavm

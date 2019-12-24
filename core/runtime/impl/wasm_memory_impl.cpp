/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_memory_impl.hpp"

namespace kagome::runtime {
  WasmMemoryImpl::WasmMemoryImpl(wasm::ShellExternalInterface::Memory *memory,
                                 SizeType size)
      : memory_(memory),
        size_(size),
        offset_{1}  // We should allocate very first byte to prohibit allocating
                    // memory at 0 in future, as returning 0 from allocate
                    // method means that wasm memory was exhausted
  {
    resize(size_);
  }

  SizeType WasmMemoryImpl::size() const {
    return size_;
  }

  void WasmMemoryImpl::resize(runtime::SizeType new_size) {
    size_ = new_size;
    return memory_->resize(new_size);
  }

  WasmPointer WasmMemoryImpl::allocate(SizeType size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = ptr + size;

    if (ptr < 0) {
      // very bad state, which should be possible, but the check must be done
      // before the cast
      return 0;
    }
    if (new_offset < static_cast<const uint32_t>(ptr)) {  // overflow
      return 0;
    }
    if (new_offset <= size_) {
      offset_ = new_offset;
      allocated_[ptr] = size;
      return ptr;
    }

    return freealloc(size);
  }

  boost::optional<SizeType> WasmMemoryImpl::deallocate(WasmPointer ptr) {
    const auto it = allocated_.find(ptr);
    if (it == allocated_.end()) {
      return boost::none;
    }
    const auto size = it->second;

    allocated_.erase(ptr);
    deallocated_[ptr] = size;

    return size;
  }

  WasmPointer WasmMemoryImpl::freealloc(SizeType size) {
    auto ptr = findContaining(size);
    if (ptr == 0) {
      // if did not find available space among deallocated memory chunks, then
      // grow memory and allocate in new space
      return growAlloc(size);
    }
    deallocated_.erase(ptr);
    allocated_[ptr] = size;
    return ptr;
  }

  WasmPointer WasmMemoryImpl::findContaining(SizeType size) {
    auto min_value = std::numeric_limits<WasmPointer>::max();
    WasmPointer min_key = 0;
    for (const auto &[key, value] : deallocated_) {
      if (min_value < 0) {
        return 0;
      }
      if (value < static_cast<uint32_t>(min_value) and value >= size) {
        min_value = value;
        min_key = key;
      }
    }
    return min_key;
  }

  WasmPointer WasmMemoryImpl::growAlloc(SizeType size) {
    if (offset_ < 0) {
      return 0;
    }

    // check that we do not exceed max memory size
    if (static_cast<uint32_t>(offset_) > kMaxMemorySize - size) {
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

  int8_t WasmMemoryImpl::load8s(WasmPointer addr) const {
    return memory_->get<int8_t>(addr);
  }
  uint8_t WasmMemoryImpl::load8u(WasmPointer addr) const {
    return memory_->get<uint8_t>(addr);
  }
  int16_t WasmMemoryImpl::load16s(WasmPointer addr) const {
    return memory_->get<int16_t>(addr);
  }
  uint16_t WasmMemoryImpl::load16u(WasmPointer addr) const {
    return memory_->get<uint16_t>(addr);
  }
  int32_t WasmMemoryImpl::load32s(WasmPointer addr) const {
    return memory_->get<int32_t>(addr);
  }
  uint32_t WasmMemoryImpl::load32u(WasmPointer addr) const {
    return memory_->get<uint32_t>(addr);
  }
  int64_t WasmMemoryImpl::load64s(WasmPointer addr) const {
    return memory_->get<int64_t>(addr);
  }
  uint64_t WasmMemoryImpl::load64u(WasmPointer addr) const {
    return memory_->get<uint64_t>(addr);
  }
  std::array<uint8_t, 16> WasmMemoryImpl::load128(WasmPointer addr) const {
    return memory_->get<std::array<uint8_t, 16>>(addr);
  }

  common::Buffer WasmMemoryImpl::loadN(kagome::runtime::WasmPointer addr,
                                       kagome::runtime::SizeType n) const {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    common::Buffer res;
    for (auto i = addr; i < addr + n; i++) {
      res.putUint8(memory_->get<uint8_t>(i));
    }
    return res;
  }

  void WasmMemoryImpl::store8(WasmPointer addr, int8_t value) {
    memory_->set<int8_t>(addr, value);
  }
  void WasmMemoryImpl::store16(WasmPointer addr, int16_t value) {
    memory_->set<int16_t>(addr, value);
  }
  void WasmMemoryImpl::store32(WasmPointer addr, int32_t value) {
    memory_->set<int32_t>(addr, value);
  }
  void WasmMemoryImpl::store64(WasmPointer addr, int64_t value) {
    memory_->set<int64_t>(addr, value);
  }
  void WasmMemoryImpl::store128(WasmPointer addr,
                                const std::array<uint8_t, 16> &value) {
    memory_->set<std::array<uint8_t, 16>>(addr, value);
  }
  void WasmMemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                                   const kagome::common::Buffer &value) {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    // boundaries, 04.04.2019
    const auto &value_vector = value.toVector();
    for (size_t i = addr, j = 0; i < addr + value.size(); i++, j++) {
      memory_->set(i, value_vector[j]);
    }
  }

}  // namespace kagome::runtime

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/wasm_memory_impl.hpp"

namespace kagome::runtime {

  WasmMemoryImpl::WasmMemoryImpl()
      : offset_{1}  // We should allocate very first byte to prohibit allocating
                    // memory at 0 in future, as returning 0 from allocate
                    // method means that wasm memory was exhausted
  {
    resizeInternal(kMaxMemorySize / 2);
  }

  WasmMemoryImpl::WasmMemoryImpl(SizeType size) : WasmMemoryImpl() {
    resizeInternal(size);
  }

  SizeType WasmMemoryImpl::size() const {
    return memory_.size();
  }

  void WasmMemoryImpl::resize(runtime::SizeType new_size) {
    return resizeInternal(new_size);
  }

  void WasmMemoryImpl::resizeInternal(SizeType new_size) {
    // Ensure the smallest allocation is large enough that most allocators
    // will provide page-aligned storage. This hopefully allows the
    // interpreter's memory to be as aligned as the MemoryImpl being
    // simulated, ensuring that the performance doesn't needlessly degrade.
    //
    // The code is optimistic this will work until WG21's p0035r0 happens.
    const size_t minSize = 1 << 12;
    size_t oldSize = memory_.size();
    memory_.resize(std::max(minSize, static_cast<size_t>(new_size)));
    if (new_size < oldSize && new_size < minSize) {
      std::memset(&memory_[new_size], 0, minSize - new_size);
    }
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
    if (new_offset <= memory_.size()) {
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
    // try to increase memory size up to offset + size * 4 (we multiply by 4 to
    // have more memory than currently needed to avoid resizing every time when
    // we exceed current memory)
    if (static_cast<uint32_t>(offset_) < kMaxMemorySize - size * 4) {
      memory_.resize(offset_ + size * 4);
    } else {
      // if we can't increase by size * 4 then increase memory size by provided
      // size
      memory_.resize(offset_ + size);
    }
    return allocate(size);
  }

  int8_t WasmMemoryImpl::load8s(WasmPointer addr) const {
    return get<int8_t>(addr);
  }
  uint8_t WasmMemoryImpl::load8u(WasmPointer addr) const {
    return get<uint8_t>(addr);
  }
  int16_t WasmMemoryImpl::load16s(WasmPointer addr) const {
    return get<int16_t>(addr);
  }
  uint16_t WasmMemoryImpl::load16u(WasmPointer addr) const {
    return get<uint16_t>(addr);
  }
  int32_t WasmMemoryImpl::load32s(WasmPointer addr) const {
    return get<int32_t>(addr);
  }
  uint32_t WasmMemoryImpl::load32u(WasmPointer addr) const {
    return get<uint32_t>(addr);
  }
  int64_t WasmMemoryImpl::load64s(WasmPointer addr) const {
    return get<int64_t>(addr);
  }
  uint64_t WasmMemoryImpl::load64u(WasmPointer addr) const {
    return get<uint64_t>(addr);
  }
  std::array<uint8_t, 16> WasmMemoryImpl::load128(WasmPointer addr) const {
    return get<std::array<uint8_t, 16>>(addr);
  }

  common::Buffer WasmMemoryImpl::loadN(kagome::runtime::WasmPointer addr,
                                       kagome::runtime::SizeType n) const {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    // boundaries, 04.04.2019
    auto first = memory_.begin() + addr;
    auto last = first + n;
    return common::Buffer(std::vector<uint8_t>(first, last));
  }

  void WasmMemoryImpl::store8(WasmPointer addr, int8_t value) {
    set<int8_t>(addr, value);
  }
  void WasmMemoryImpl::store16(WasmPointer addr, int16_t value) {
    set<int16_t>(addr, value);
  }
  void WasmMemoryImpl::store32(WasmPointer addr, int32_t value) {
    set<int32_t>(addr, value);
  }
  void WasmMemoryImpl::store64(WasmPointer addr, int64_t value) {
    set<int64_t>(addr, value);
  }
  void WasmMemoryImpl::store128(WasmPointer addr,
                                const std::array<uint8_t, 16> &value) {
    set<std::array<uint8_t, 16>>(addr, value);
  }
  void WasmMemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                                   const kagome::common::Buffer &value) {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    // boundaries, 04.04.2019
    const auto &value_vector = value.toVector();
    memory_.insert(
        memory_.begin() + addr, value_vector.begin(), value_vector.end());
  }

}  // namespace kagome::runtime

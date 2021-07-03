/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/memory.hpp"

namespace kagome::runtime::wavm {

  Memory::Memory(WAVM::Runtime::Memory *memory, WasmSize heap_base)
      : memory_(memory),
        heap_base_{heap_base},
        offset_{heap_base_},
        logger_{log::createLogger("WavmMemory", "runtime")} {
    BOOST_ASSERT(memory_);
    BOOST_ASSERT(heap_base_ > 0);

    resize(kInitialMemorySize);
  }

  WasmPointer Memory::allocate(WasmSize size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = roundUpAlign(ptr + size);  // align
    size = new_offset - ptr;

    BOOST_ASSERT(allocated_.find(ptr) == allocated_.end());
    if (kMaxMemorySize - offset_ < size) {  // overflow
      logger_->error(
          "overflow occurred while trying to allocate {} bytes at offset "
          "0x{:x}",
          size,
          offset_);
      return 0;
    }
    if (new_offset <= this->size()) {
      offset_ = new_offset;
      allocated_[ptr] = size;
      SL_TRACE_FUNC_CALL(logger_, ptr, this, size);
      return ptr;
    }

    auto res = freealloc(size);
    SL_TRACE_FUNC_CALL(logger_, res, this, size);
    return res;
  }

  boost::optional<WasmSize> Memory::deallocate(WasmPointer ptr) {
    const auto it = allocated_.find(ptr);
    if (it == allocated_.end()) {
      return boost::none;
    }

    auto a_node = allocated_.extract(it);
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

  WasmPointer Memory::freealloc(WasmSize size) {
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

  WasmPointer Memory::growAlloc(WasmSize size) {
    // check that we do not exceed max memory size
    if (kMaxMemorySize - offset_ < size) {
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
    auto byte_array = loadArray<uint8_t>(addr, 16);
    std::array<uint8_t, 16> array;
    std::copy_n(byte_array, 16, array.begin());
    return array;
  }

  common::Buffer Memory::loadN(kagome::runtime::WasmPointer addr,
                               kagome::runtime::WasmSize n) const {
    common::Buffer res;
    auto byte_array = loadArray<uint8_t>(addr, n);
    return common::Buffer{byte_array, byte_array + n};
  }

  std::string Memory::loadStr(kagome::runtime::WasmPointer addr,
                              kagome::runtime::WasmSize n) const {
    std::string res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.push_back(load<uint8_t>(i));
    }
    SL_TRACE_FUNC_CALL(logger_, res, this, addr, n);
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
    storeBuffer(addr, value);
  }
  void Memory::storeBuffer(kagome::runtime::WasmPointer addr,
                           gsl::span<const uint8_t> value) {
    storeArray(addr, value);
  }

  WasmSpan Memory::storeBuffer(gsl::span<const uint8_t> value) {
    auto wasm_pointer = allocate(value.size());
    if (wasm_pointer == 0) {
      return 0;
    }
    storeBuffer(wasm_pointer, value);
    auto res = PtrSize(wasm_pointer, value.size()).combine();
    return res;
  }


  boost::optional<WasmSize> Memory::getDeallocatedChunkSize(
      WasmPointer ptr) const {
    auto it = deallocated_.find(ptr);
    return it != deallocated_.cend() ? boost::make_optional(it->second)
                                     : boost::none;
  }

  boost::optional<WasmSize> Memory::getAllocatedChunkSize(
      WasmPointer ptr) const {
    auto it = allocated_.find(ptr);
    return it != allocated_.cend() ? boost::make_optional(it->second)
                                   : boost::none;
  }

  size_t Memory::getAllocatedChunksNum() const {
    return allocated_.size();
  }

  size_t Memory::getDeallocatedChunksNum() const {
    return deallocated_.size();
  }

}  // namespace kagome::runtime::wavm

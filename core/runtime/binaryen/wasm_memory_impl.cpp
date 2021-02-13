/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/wasm_memory_impl.hpp"

#include "runtime/wasm_result.hpp"

namespace kagome::runtime::binaryen {
  WasmMemoryImpl::WasmMemoryImpl(wasm::ShellExternalInterface::Memory *memory,
                                 WasmSize size)
      : memory_(memory),
        size_(size),
        logger_{common::createLogger("WASM Memory")},
        heap_base_{roundUpAlign(1024u)},
        offset_{heap_base_}
  // We should allocate very first byte to prohibit allocating memory at 0 in
  // future, as returning 0 from allocate method means that wasm memory was
  // exhausted
  {
    size_ = std::max(size_, offset_);

    WasmMemoryImpl::resize(size_);
  }

  void WasmMemoryImpl::setHeapBase(WasmSize heap_base) {
    BOOST_ASSERT(heap_base_ > 0);
    BOOST_ASSERT(heap_base_ == roundUpAlign(heap_base_));
    heap_base_ = heap_base;
  }

  void WasmMemoryImpl::reset() {
    offset_ = heap_base_;
    allocated_.clear();
    deallocated_.clear();
    if (size_ < offset_) {
      size_ = offset_;
      resize(size_);
    }
    logger_->trace("Memory reset");
  }

  WasmSize WasmMemoryImpl::size() const {
    return size_;
  }

  void WasmMemoryImpl::resize(runtime::WasmSize new_size) {
    /**
     * We use this condition to avoid deallocated_ pointers fixup
     */
    BOOST_ASSERT(offset_ <= kMaxMemorySize - new_size);
    if (new_size >= size_) {
      size_ = new_size;
      memory_->resize(new_size);
    }
  }

  WasmPointer WasmMemoryImpl::allocate(WasmSize size) {
    if (size == 0) {
      return 0;
    }
    const auto ptr = offset_;
    const auto new_offset = roundUpAlign(ptr + size);  // align

    // Round up allocating chunk of memory
    size = new_offset - ptr;

    BOOST_ASSERT(allocated_.find(ptr) == allocated_.end());
    if (kMaxMemorySize - offset_ < size) {  // overflow
      logger_->error(
          "overflow occured while trying to allocate {} bytes at offset 0x{:x}",
          size,
          offset_);
      return 0;
    }
    if (new_offset <= size_) {
      offset_ = new_offset;
      allocated_[ptr] = size;
      return ptr;
    }

    return freealloc(size);
  }

  boost::optional<WasmSize> WasmMemoryImpl::deallocate(WasmPointer ptr) {
    auto a_it = allocated_.find(ptr);
    if (a_it == allocated_.end()) {
      return boost::none;
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
      auto d_it_prev = d_it;
      --d_it_prev;
      if (d_it_prev->first + d_it_prev->second != d_it->first) {
        break;
      }
      d_it_prev->second += d_it->second;
      deallocated_.erase(d_it);
      d_it = d_it_prev;
    }

    auto d_it_next = d_it;
    ++d_it_next;
    if (d_it_next == deallocated_.end()) {
      if (d_it->first + d_it->second == offset_) {
        offset_ = d_it->first;
        deallocated_.erase(d_it);
      }
    }

    return size;
  }

  WasmPointer WasmMemoryImpl::freealloc(WasmSize size) {
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

  WasmPointer WasmMemoryImpl::growAlloc(WasmSize size) {
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
    if ((kMaxMemorySize - offset_) / 4 > size) {
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
                                       kagome::runtime::WasmSize n) const {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    common::Buffer res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.putUint8(memory_->get<uint8_t>(i));
    }
    return res;
  }

  std::string WasmMemoryImpl::loadStr(kagome::runtime::WasmPointer addr,
                                      kagome::runtime::WasmSize n) const {
    std::string res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.push_back(memory_->get<uint8_t>(i));
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
                                   gsl::span<const uint8_t> value) {
    // TODO (kamilsa) PRE-98: check if we do not go outside of memory
    // boundaries, 04.04.2019
    for (size_t i = addr, j = 0; i < addr + static_cast<size_t>(value.size());
         i++, j++) {
      memory_->set(i, value[j]);
    }
  }

  WasmSpan WasmMemoryImpl::storeBuffer(gsl::span<const uint8_t> value) {
    auto wasm_pointer = allocate(value.size());
    if (wasm_pointer == 0) {
      return 0;
    }
    storeBuffer(wasm_pointer, value);
    return WasmResult(wasm_pointer, value.size()).combine();
  }

}  // namespace kagome::runtime::binaryen

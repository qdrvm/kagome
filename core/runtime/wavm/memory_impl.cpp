/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/memory_impl.hpp"

#include "runtime/common/memory_allocator.hpp"

namespace kagome::runtime::wavm {

  MemoryImpl::MemoryImpl(WAVM::Runtime::Memory *memory,
                         std::unique_ptr<MemoryAllocator> &&allocator)
      : allocator_{std::move(allocator)},
        memory_{memory},
        logger_{log::createLogger("WAVM Memory", "wavm")} {
    BOOST_ASSERT(memory_);
    BOOST_ASSERT(allocator_);
    resize(kInitialMemorySize);
  }

  MemoryImpl::MemoryImpl(WAVM::Runtime::Memory *memory, WasmSize heap_base)
      : MemoryImpl{memory,
                   std::make_unique<MemoryAllocator>(
                       MemoryAllocator::MemoryHandle{
                           [this](auto size) { return resize(size); },
                           [this]() { return size(); }},
                       kInitialMemorySize,
                       heap_base)} {}

  WasmPointer MemoryImpl::allocate(WasmSize size) {
    return allocator_->allocate(size);
  }

  boost::optional<WasmSize> MemoryImpl::deallocate(WasmPointer ptr) {
    return allocator_->deallocate(ptr);
  }

  int8_t MemoryImpl::load8s(WasmPointer addr) const {
    return load<int8_t>(addr);
  }
  uint8_t MemoryImpl::load8u(WasmPointer addr) const {
    return load<uint8_t>(addr);
  }
  int16_t MemoryImpl::load16s(WasmPointer addr) const {
    return load<int16_t>(addr);
  }
  uint16_t MemoryImpl::load16u(WasmPointer addr) const {
    return load<uint16_t>(addr);
  }
  int32_t MemoryImpl::load32s(WasmPointer addr) const {
    return load<int32_t>(addr);
  }
  uint32_t MemoryImpl::load32u(WasmPointer addr) const {
    return load<uint32_t>(addr);
  }
  int64_t MemoryImpl::load64s(WasmPointer addr) const {
    return load<int64_t>(addr);
  }
  uint64_t MemoryImpl::load64u(WasmPointer addr) const {
    return load<uint64_t>(addr);
  }
  std::array<uint8_t, 16> MemoryImpl::load128(WasmPointer addr) const {
    auto byte_array = loadArray<uint8_t>(addr, 16);
    std::array<uint8_t, 16> array;
    std::copy_n(byte_array, 16, array.begin());
    return array;
  }

  common::Buffer MemoryImpl::loadN(kagome::runtime::WasmPointer addr,
                                   kagome::runtime::WasmSize n) const {
    common::Buffer res;
    auto byte_array = loadArray<uint8_t>(addr, n);
    return common::Buffer{byte_array, byte_array + n};
  }

  std::string MemoryImpl::loadStr(kagome::runtime::WasmPointer addr,
                                  kagome::runtime::WasmSize n) const {
    std::string res;
    res.reserve(n);
    for (auto i = addr; i < addr + n; i++) {
      res.push_back(load<uint8_t>(i));
    }
    SL_TRACE_FUNC_CALL(logger_, res, this, addr, n);
    return res;
  }

  void MemoryImpl::store8(WasmPointer addr, int8_t value) {
    store<int8_t>(addr, value);
  }
  void MemoryImpl::store16(WasmPointer addr, int16_t value) {
    store<int16_t>(addr, value);
  }
  void MemoryImpl::store32(WasmPointer addr, int32_t value) {
    store<int32_t>(addr, value);
  }
  void MemoryImpl::store64(WasmPointer addr, int64_t value) {
    store<int64_t>(addr, value);
  }
  void MemoryImpl::store128(WasmPointer addr,
                            const std::array<uint8_t, 16> &value) {
    storeBuffer(addr, value);
  }
  void MemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                               gsl::span<const uint8_t> value) {
    storeArray(addr, value);
  }

  WasmSpan MemoryImpl::storeBuffer(gsl::span<const uint8_t> value) {
    auto wasm_pointer = allocate(value.size());
    if (wasm_pointer == 0) {
      return 0;
    }
    storeBuffer(wasm_pointer, value);
    auto res = PtrSize(wasm_pointer, value.size()).combine();
    return res;
  }

}  // namespace kagome::runtime::wavm

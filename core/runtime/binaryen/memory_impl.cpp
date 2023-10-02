/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/memory_impl.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::runtime::binaryen {

  MemoryImpl::MemoryImpl(RuntimeExternalInterface::InternalMemory *memory,
                         const MemoryConfig &config)
      : memory_{memory},
        allocator_{std::make_unique<MemoryAllocator>(
            MemoryAllocator::MemoryHandle{
                [this](auto new_size) { return resize(new_size); },
                [this]() { return size(); },
                [this](auto addr, uint32_t value) {
                  memory_->set<uint32_t>(addr, value);
                },
                [this](auto addr) { return memory_->get<uint32_t>(addr); }},
            config)},
        logger_{log::createLogger("Binaryen Memory", "binaryen")} {
    // TODO(Harrm): #1714 temporary fix because binaryen doesn't recognize
    // our memory resizes from our allocator
    memory_->resize(kInitialMemorySize);
  }

  WasmPointer MemoryImpl::allocate(WasmSize size) {
    return allocator_->allocate(size);
  }

  std::optional<WasmSize> MemoryImpl::deallocate(WasmPointer ptr) {
    return allocator_->deallocate(ptr);
  }

  int8_t MemoryImpl::load8s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int8_t>(addr));
    return memory_->get<int8_t>(addr);
  }
  uint8_t MemoryImpl::load8u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint8_t>(addr));
    return memory_->get<uint8_t>(addr);
  }
  int16_t MemoryImpl::load16s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int16_t>(addr));
    return memory_->get<int16_t>(addr);
  }
  uint16_t MemoryImpl::load16u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint16_t>(addr));
    return memory_->get<uint16_t>(addr);
  }
  int32_t MemoryImpl::load32s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int32_t>(addr));
    return memory_->get<int32_t>(addr);
  }
  uint32_t MemoryImpl::load32u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint32_t>(addr));
    return memory_->get<uint32_t>(addr);
  }
  int64_t MemoryImpl::load64s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int64_t>(addr));
    return memory_->get<int64_t>(addr);
  }
  uint64_t MemoryImpl::load64u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint64_t>(addr));
    return memory_->get<uint64_t>(addr);
  }
  std::array<uint8_t, 16> MemoryImpl::load128(WasmPointer addr) const {
    BOOST_ASSERT((allocator_->checkAddress<std::array<uint8_t, 16>>(addr)));
    return memory_->get<std::array<uint8_t, 16>>(addr);
  }

  common::BufferView MemoryImpl::loadN(kagome::runtime::WasmPointer addr,
                                       kagome::runtime::WasmSize n) const {
    BOOST_ASSERT(size() > addr and size() - addr >= n);
    SL_INFO(logger_, "load buffer with size {} at {}", n, addr);
    return common::BufferView{memory_->getBuffer<const uint8_t>(addr, n)};
  }

  std::string MemoryImpl::loadStr(kagome::runtime::WasmPointer addr,
                                  kagome::runtime::WasmSize length) const {
    BOOST_ASSERT(size() > addr and size() - addr >= length);
    std::string res;
    res.reserve(length);
    for (auto i = addr; i < addr + length; i++) {
      res.push_back(static_cast<char>(memory_->get<uint8_t>(i)));
    }
    SL_INFO(logger_, "load buffer with size {} at {}", length, addr);
    return res;
  }

  void MemoryImpl::store8(WasmPointer addr, int8_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int8_t>(addr)));
    memory_->set<int8_t>(addr, value);
  }

  void MemoryImpl::store16(WasmPointer addr, int16_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int16_t>(addr)));
    memory_->set<int16_t>(addr, value);
  }

  void MemoryImpl::store32(WasmPointer addr, int32_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int32_t>(addr)));
    memory_->set<int32_t>(addr, value);
  }

  void MemoryImpl::store64(WasmPointer addr, int64_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int64_t>(addr)));
    memory_->set<int64_t>(addr, value);
  }

  void MemoryImpl::store128(WasmPointer addr,
                            const std::array<uint8_t, 16> &value) {
    BOOST_ASSERT((allocator_->checkAddress<std::array<uint8_t, 16>>(addr)));
    memory_->set<std::array<uint8_t, 16>>(addr, value);
  }

  void MemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                               gsl::span<const uint8_t> value) {
    BOOST_ASSERT(
        (allocator_->checkAddress(addr, static_cast<size_t>(value.size()))));
    memory_->set(addr, std::move(value));
    SL_INFO(logger_, "store buffer with size {} at {}", value.size(), addr);
  }

  WasmSpan MemoryImpl::storeBuffer(gsl::span<const uint8_t> value) {
    const auto size = static_cast<size_t>(value.size());
    BOOST_ASSERT(std::numeric_limits<WasmSize>::max() > size);
    auto wasm_pointer = allocate(size);
    if (wasm_pointer == 0) {
      return 0;
    }
    storeBuffer(wasm_pointer, value);
    return PtrSize(wasm_pointer, value.size()).combine();
  }

}  // namespace kagome::runtime::binaryen

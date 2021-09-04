/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_impl.hpp"

#include "runtime/common/memory_allocator.hpp"
#include "runtime/ptr_size.hpp"

#include <wasmedge.h>

namespace {
  template <typename T, auto N = sizeof(T)>
  auto toArray(T t) {
    std::array<uint8_t, N> res;
    for (unsigned i = 0; i < N; ++i) {
      res[i] = static_cast<unsigned char &&>((t >> (N - i - 1) * 8) & 0xFF);
    }
    return res;
  }
}  // namespace

namespace kagome::runtime::wasmedge {

  MemoryImpl::MemoryImpl(wasm::WasmEdge_MemoryInstanceContext *memory,
                         std::unique_ptr<MemoryAllocator> &&allocator)
      : memory_{memory},
        size_{kInitialMemorySize},
        allocator_{std::move(allocator)},
        logger_{log::createLogger("Binaryen Memory", "binaryen")} {
    resize(size_);
  }

  MemoryImpl::MemoryImpl(wasm::ShellExternalInterface::Memory *memory,
                         WasmSize heap_base)
      : MemoryImpl{memory,
                   std::make_unique<MemoryAllocator>(
                       MemoryAllocator::MemoryHandle{
                           [this](auto new_size) { return resize(new_size); },
                           [this]() { return size_; }},
                       kInitialMemorySize,
                       heap_base)} {}

  WasmPointer MemoryImpl::allocate(WasmSize size) {
    return allocator_->allocate(size);
  }

  boost::optional<WasmSize> MemoryImpl::deallocate(WasmPointer ptr) {
    return allocator_->deallocate(ptr);
  }

  common::Buffer MemoryImpl::loadN(kagome::runtime::WasmPointer addr,
                                   kagome::runtime::WasmSize n) const {
    BOOST_ASSERT(size_ > addr and size_ - addr >= n);
    common::Buffer res(n, 0);
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, n);
    return res;
  }

  std::string MemoryImpl::loadStr(kagome::runtime::WasmPointer addr,
                                  kagome::runtime::WasmSize length) const {
    BOOST_ASSERT(size_ > addr and size_ - addr >= length);
    std::string res(length, '\0');
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, length);
    return res;
  }

  void MemoryImpl::store8(WasmPointer addr, int8_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int8_t>(addr)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, toArray(value).data(), addr, sizeof(value));
  }

  void MemoryImpl::store16(WasmPointer addr, int16_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int16_t>(addr)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, toArray(value).data(), addr, sizeof(value));
  }

  void MemoryImpl::store32(WasmPointer addr, int32_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int32_t>(addr)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, toArray(value).data(), addr, sizeof(value));
  }

  void MemoryImpl::store64(WasmPointer addr, int64_t value) {
    BOOST_ASSERT((allocator_->checkAddress<int64_t>(addr)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, toArray(value).data(), addr, sizeof(value));
  }

  void MemoryImpl::store128(WasmPointer addr,
                            const std::array<uint8_t, 16> &value) {
    BOOST_ASSERT((allocator_->checkAddress<std::array<uint8_t, 16>>(addr)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, value.data(), addr, sizeof(value));
  }

  void MemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                               gsl::span<const uint8_t> value) {
    const auto size = static_cast<size_t>(value.size());
    BOOST_ASSERT((allocator_->checkAddress(addr, size)));
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, value.data(), addr, size);
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

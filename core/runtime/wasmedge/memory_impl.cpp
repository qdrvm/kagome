/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/memory_impl.hpp"

#include "common/literals.hpp"
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

  template <typename T, auto N = sizeof(T)>
  auto fromArray(std::array<uint8_t, N> a) {
    T res{0};
    for (unsigned i = 0; i < N; ++i) {
      res |= static_cast<T>(a[i]) << (i * 8);
    }
    return res;
  }
}  // namespace

namespace kagome::runtime::wasmedge {

  using namespace kagome::common::literals;

  MemoryImpl::MemoryImpl(WasmEdge_MemoryInstanceContext *memory,
                         std::unique_ptr<MemoryAllocator> &&allocator)
      : memory_{memory},
        size_{kInitialMemorySize},
        allocator_{std::move(allocator)},
        logger_{log::createLogger("WasmEdge Memory", "wasmedge")} {
    resize(size_);
  }

  MemoryImpl::MemoryImpl(WasmEdge_MemoryInstanceContext *memory)
      : MemoryImpl{memory,
                   std::make_unique<MemoryAllocator>(
                       MemoryAllocator::MemoryHandle{
                           [this](auto new_size) { return resize(new_size); },
                           [this]() { return size_; }},
                       kInitialMemorySize,
                       2000000)} {}

  WasmPointer MemoryImpl::allocate(WasmSize size) {
    return allocator_->allocate(size);
  }

  boost::optional<WasmSize> MemoryImpl::deallocate(WasmPointer ptr) {
    return allocator_->deallocate(ptr);
  }

  int8_t MemoryImpl::load8s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int8_t>(addr));
    int8_t res;
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, reinterpret_cast<uint8_t *>(&res), addr, sizeof(res));
    return res;
  }
  uint8_t MemoryImpl::load8u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint8_t>(addr));
    uint8_t res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, &res, addr, sizeof(res));
    return res;
  }
  int16_t MemoryImpl::load16s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int16_t>(addr));
    std::array<uint8_t, 2> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<int16_t>(res);
  }
  uint16_t MemoryImpl::load16u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint16_t>(addr));
    std::array<uint8_t, 2> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<uint16_t>(res);
  }
  int32_t MemoryImpl::load32s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int32_t>(addr));
    std::array<uint8_t, 4> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<int32_t>(res);
  }
  uint32_t MemoryImpl::load32u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint32_t>(addr));
    std::array<uint8_t, 4> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<uint32_t>(res);
  }
  int64_t MemoryImpl::load64s(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<int64_t>(addr));
    std::array<uint8_t, 8> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<int64_t>(res);
  }
  uint64_t MemoryImpl::load64u(WasmPointer addr) const {
    BOOST_ASSERT(allocator_->checkAddress<uint64_t>(addr));
    std::array<uint8_t, 8> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return fromArray<uint64_t>(res);
  }
  std::array<uint8_t, 16> MemoryImpl::load128(WasmPointer addr) const {
    BOOST_ASSERT((allocator_->checkAddress<std::array<uint8_t, 16>>(addr)));
    std::array<uint8_t, 16> res;
    WasmEdge_Result Res =
        WasmEdge_MemoryInstanceGetData(memory_, res.data(), addr, sizeof(res));
    return res;
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
    WasmEdge_Result Res = WasmEdge_MemoryInstanceGetData(
        memory_, reinterpret_cast<uint8_t *>(res.data()), addr, length);
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
        memory_, const_cast<uint8_t *>(value.data()), addr, sizeof(value));
  }

  void MemoryImpl::storeBuffer(kagome::runtime::WasmPointer addr,
                               gsl::span<const uint8_t> value) {
    const auto size = static_cast<size_t>(value.size());
    BOOST_ASSERT((allocator_->checkAddress(addr, size)));
    WasmEdge_Result Res = WasmEdge_MemoryInstanceSetData(
        memory_, const_cast<uint8_t *>(value.data()), addr, size);
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

}  // namespace kagome::runtime::wasmedge

/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasm_edge/memory_impl.hpp"

namespace kagome::runtime::wasm_edge {

  MemoryImpl::MemoryImpl(WasmEdge_MemoryInstanceContext *mem_instance,
                         const MemoryConfig &config)
      : mem_instance_{std::move(mem_instance)},
        allocator_{MemoryAllocator{
            MemoryAllocator::MemoryHandle{
                .resize = [this](size_t new_size) { resize(new_size); },
                .getSize = [this]() -> size_t { return size(); },
                .storeSz = [this](WasmPointer p, uint32_t n) { store32(p, n); },
                .loadSz = [this](WasmPointer p) -> uint32_t {
                  return load32u(p);
                }},
            config}} {
    BOOST_ASSERT(mem_instance_ != nullptr);
    SL_DEBUG(logger_,
             "Created memory wrapper {} for internal instance {}",
             fmt::ptr(this),
             fmt::ptr(mem_instance_));
  }

  void MemoryImpl::resize(WasmSize new_size) {
    if (new_size > size()) {
      auto old_page_num = WasmEdge_MemoryInstanceGetPageSize(mem_instance_);
      auto new_page_num = (new_size + kMemoryPageSize - 1) / kMemoryPageSize;
      [[maybe_unused]] auto res = WasmEdge_MemoryInstanceGrowPage(
          mem_instance_, new_page_num - old_page_num);
      BOOST_ASSERT(WasmEdge_ResultOK(res));
      SL_DEBUG(logger_,
               "Grow memory to {} pages ({} bytes) - {}",
               new_page_num,
               new_size,
               WasmEdge_ResultGetMessage(res));
    }
  }

  ExternalMemoryProviderImpl::ExternalMemoryProviderImpl(
      WasmEdge_MemoryInstanceContext *wasmedge_memory)
      : wasmedge_memory_{wasmedge_memory} {
    BOOST_ASSERT(wasmedge_memory_);
  }

  std::optional<std::reference_wrapper<runtime::Memory>>
  ExternalMemoryProviderImpl::getCurrentMemory() const {
    if (current_memory_) {
      return std::reference_wrapper<runtime::Memory>(**current_memory_);
    }
    return std::nullopt;
  }

  [[nodiscard]] outcome::result<void> ExternalMemoryProviderImpl::resetMemory(
      const MemoryConfig &config) {
    current_memory_ = std::make_shared<MemoryImpl>(wasmedge_memory_, config);
    return outcome::success();
  }

  std::optional<std::reference_wrapper<runtime::Memory>>
  InternalMemoryProviderImpl::getCurrentMemory() const {
    if (current_memory_) {
      return std::reference_wrapper<runtime::Memory>(**current_memory_);
    }
    return std::nullopt;
  }

  [[nodiscard]] outcome::result<void> InternalMemoryProviderImpl::resetMemory(
      const MemoryConfig &config) {
    if (wasmedge_memory_) {
      current_memory_ = std::make_shared<MemoryImpl>(wasmedge_memory_, config);
    }
    return outcome::success();
  }

  void InternalMemoryProviderImpl::setMemory(
      WasmEdge_MemoryInstanceContext *wasmedge_memory) {
    wasmedge_memory_ = wasmedge_memory;
  }

}  // namespace kagome::runtime::wasm_edge

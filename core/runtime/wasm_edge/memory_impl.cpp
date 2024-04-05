/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasm_edge/memory_impl.hpp"
#include "runtime/common/memory_error.hpp"
#include "runtime/memory_check.hpp"

namespace kagome::runtime::wasm_edge {

  MemoryImpl::MemoryImpl(WasmEdge_MemoryInstanceContext *mem_instance,
                         const MemoryConfig &config)
      : mem_instance_{std::move(mem_instance)},
        allocator_{MemoryAllocator{*this, config}} {
    BOOST_ASSERT(mem_instance_ != nullptr);
    SL_DEBUG(logger_,
             "Created memory wrapper {} for internal instance {}",
             fmt::ptr(this),
             fmt::ptr(mem_instance_));
  }

  std::optional<WasmSize> MemoryImpl::pagesMax() const {
    auto type = WasmEdge_MemoryInstanceGetMemoryType(mem_instance_);
    if (type == nullptr) {
      throw std::runtime_error{
          "WasmEdge_MemoryInstanceGetMemoryType returned nullptr"};
    }
    auto limit = WasmEdge_MemoryTypeGetLimit(type);
    return limit.HasMax ? std::make_optional(limit.Max) : std::nullopt;
  }

  void MemoryImpl::resize(WasmSize new_size) {
    if (new_size > size()) {
      auto old_page_num = WasmEdge_MemoryInstanceGetPageSize(mem_instance_);
      auto new_page_num = sizeToPages(new_size);
      auto res = WasmEdge_MemoryInstanceGrowPage(mem_instance_,
                                                 new_page_num - old_page_num);
      if (not WasmEdge_ResultOK(res)) {
        throw std::runtime_error{fmt::format(
            "WasmEdge_MemoryInstanceGrowPage: Failed to grow page num: {}",
            WasmEdge_ResultGetMessage(res))};
      }
      SL_DEBUG(logger_,
               "Grow memory to {} pages ({} bytes) - {}",
               new_page_num,
               new_size,
               WasmEdge_ResultGetMessage(res));
    }
  }

  outcome::result<BytesOut> MemoryImpl::view(WasmPointer ptr,
                                             WasmSize size) const {
    if (not memoryCheck(ptr, size, this->size())) {
      return MemoryError::ERROR;
    }
    auto raw = WasmEdge_MemoryInstanceGetPointer(mem_instance_, ptr, size);
    if (raw == nullptr) {
      throw std::runtime_error{
          "WasmEdge_MemoryInstanceGetPointer returned nullptr"};
    }
    return BytesOut{raw, size};
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

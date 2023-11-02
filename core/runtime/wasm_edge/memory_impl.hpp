/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wasmedge/wasmedge.h>

#include "log/trace_macros.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::wasm_edge {

  class MemoryImpl final : public Memory {
   public:
    MemoryImpl(WasmEdge_MemoryInstanceContext *mem_instance,
               const MemoryConfig &config)
        : mem_instance_{std::move(mem_instance)},
          allocator_{MemoryAllocator{
              MemoryAllocator::MemoryHandle{
                  .resize = [this](size_t new_size) { resize(new_size); },
                  .getSize = [this]() -> size_t { return size(); },
                  .storeSz = [this](WasmPointer p,
                                    uint32_t n) { store32(p, n); },
                  .loadSz = [this](WasmPointer p) -> uint32_t {
                    return load32u(p);
                  }},
              config}} {
      BOOST_ASSERT(mem_instance_ != nullptr);
    }
    /**
     * @brief Return the size of the memory
     */
    WasmSize size() const override {
      return WasmEdge_MemoryInstanceGetPageSize(mem_instance_)
           * kMemoryPageSize;
    }

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    void resize(WasmSize new_size) override {
      if (new_size > size()) {
        auto old_page_num = WasmEdge_MemoryInstanceGetPageSize(mem_instance_);
        auto new_page_num = (new_size + kMemoryPageSize - 1) / kMemoryPageSize;
        [[maybe_unused]] auto res = WasmEdge_MemoryInstanceGrowPage(
            mem_instance_, new_page_num - old_page_num);
        BOOST_ASSERT(WasmEdge_ResultOK(res));
      }
    }

    WasmPointer allocate(WasmSize size) override {
      return allocator_.allocate(size);
    }

    std::optional<WasmSize> deallocate(WasmPointer ptr) override {
      return allocator_.deallocate(ptr);
    }

    template <typename T>
    T loadInt(WasmPointer addr) const {
      T data{};
      [[maybe_unused]] auto res = WasmEdge_MemoryInstanceGetData(
          mem_instance_, reinterpret_cast<uint8_t *>(&data), addr, sizeof(T));
      BOOST_ASSERT(WasmEdge_ResultOK(res));
      if constexpr (std::is_integral_v<T>) {
        SL_TRACE_FUNC_CALL(logger_, data, addr);
      } else {
        SL_TRACE_FUNC_CALL(logger_, common::BufferView{data}, addr);
      }
      return data;
    }

    int8_t load8s(WasmPointer addr) const override {
      return loadInt<int8_t>(addr);
    }

    uint8_t load8u(WasmPointer addr) const override {
      return loadInt<uint8_t>(addr);
    }

    int16_t load16s(WasmPointer addr) const override {
      return loadInt<int16_t>(addr);
    }

    uint16_t load16u(WasmPointer addr) const override {
      return loadInt<uint16_t>(addr);
    }

    int32_t load32s(WasmPointer addr) const override {
      return loadInt<int32_t>(addr);
    }

    uint32_t load32u(WasmPointer addr) const override {
      return loadInt<uint32_t>(addr);
    }

    int64_t load64s(WasmPointer addr) const override {
      return loadInt<int64_t>(addr);
    }

    uint64_t load64u(WasmPointer addr) const override {
      return loadInt<uint64_t>(addr);
    }

    std::array<uint8_t, 16> load128(WasmPointer addr) const override {
      return loadInt<std::array<uint8_t, 16>>(addr);
    }

    common::BufferView loadN(WasmPointer addr, WasmSize n) const override {
      auto ptr = WasmEdge_MemoryInstanceGetPointer(mem_instance_, addr, n);
      BOOST_ASSERT(ptr);
      SL_TRACE_FUNC_CALL(logger_, fmt::ptr(ptr), addr, n);
      return common::BufferView{ptr, n};
    }

    std::string loadStr(WasmPointer addr, WasmSize n) const override {
      std::string res(n, ' ');
      auto span = loadN(addr, n);
      std::copy_n(span.begin(), n, res.begin());
      SL_TRACE_FUNC_CALL(logger_, res, addr, n);
      return res;
    }

    template <typename T>
    void storeInt(WasmPointer addr, T value) const {
      [[maybe_unused]] auto res = WasmEdge_MemoryInstanceSetData(
          mem_instance_, reinterpret_cast<uint8_t *>(&value), addr, sizeof(T));
      BOOST_ASSERT(WasmEdge_ResultOK(res));
      if constexpr (std::is_integral_v<T>) {
        SL_TRACE_FUNC_CALL(
            logger_, WasmEdge_ResultGetMessage(res), addr, value);
      } else {
        SL_TRACE_FUNC_CALL(logger_,
                           WasmEdge_ResultGetMessage(res),
                           addr,
                           common::BufferView{value});
      }
    }

    void store8(WasmPointer addr, int8_t value) override {
      storeInt<int8_t>(addr, value);
    }

    void store16(WasmPointer addr, int16_t value) override {
      storeInt<int16_t>(addr, value);
    }

    void store32(WasmPointer addr, int32_t value) override {
      storeInt<int32_t>(addr, value);
    }

    void store64(WasmPointer addr, int64_t value) override {
      storeInt<int64_t>(addr, value);
    }

    void store128(WasmPointer addr,
                  const std::array<uint8_t, 16> &value) override {
      storeBuffer(addr, value);
    }

    void storeBuffer(WasmPointer addr,
                     gsl::span<const uint8_t> value) override {
      auto res = WasmEdge_MemoryInstanceSetData(
          mem_instance_, value.data(), addr, value.size());
      if (!WasmEdge_ResultOK(res)) {
        SL_ERROR(logger_, "{}", WasmEdge_ResultGetMessage(res));
      }
      SL_TRACE_FUNC_CALL(logger_, WasmEdge_ResultGetMessage(res), addr);
    }

    /**
     * @brief allocates buffer in memory and copies value into memory
     * @param value buffer to store
     * @return full wasm pointer to allocated buffer
     */
    WasmSpan storeBuffer(gsl::span<const uint8_t> value) override {
      auto ptr = allocate(value.size());
      storeBuffer(ptr, value);
      return PtrSize{ptr, static_cast<WasmSize>(value.size())}.combine();
    }

   private:
    WasmEdge_MemoryInstanceContext *mem_instance_;
    MemoryAllocator allocator_;
    log::Logger logger_ = log::createLogger("Memory", "runtime");
  };

  class ExternalMemoryProviderImpl final : public MemoryProvider {
   public:
    explicit ExternalMemoryProviderImpl(
        WasmEdge_MemoryInstanceContext *wasmedge_memory)
        : wasmedge_memory_{wasmedge_memory} {
      BOOST_ASSERT(wasmedge_memory_);
    }
    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override {
      if (current_memory_) {
        return std::reference_wrapper<runtime::Memory>(**current_memory_);
      }
      return std::nullopt;
    }

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override {
      current_memory_ = std::make_shared<MemoryImpl>(wasmedge_memory_, config);
      return outcome::success();
    }

   private:
    std::optional<std::shared_ptr<MemoryImpl>> current_memory_;
    WasmEdge_MemoryInstanceContext *wasmedge_memory_;
  };

  class InternalMemoryProviderImpl final : public MemoryProvider {
   public:
    explicit InternalMemoryProviderImpl() = default;

    std::optional<std::reference_wrapper<runtime::Memory>> getCurrentMemory()
        const override {
      if (current_memory_) {
        return std::reference_wrapper<runtime::Memory>(**current_memory_);
      }
      return std::nullopt;
    }

    [[nodiscard]] outcome::result<void> resetMemory(
        const MemoryConfig &config) override {
      if (wasmedge_memory_) {
        current_memory_ =
            std::make_shared<MemoryImpl>(wasmedge_memory_, config);
      }
      return outcome::success();
    }

    void setMemory(WasmEdge_MemoryInstanceContext *wasmedge_memory) {
      wasmedge_memory_ = wasmedge_memory;
    }

   private:
    std::optional<std::shared_ptr<MemoryImpl>> current_memory_;
    WasmEdge_MemoryInstanceContext *wasmedge_memory_{};
  };

}  // namespace kagome::runtime::wasm_edge

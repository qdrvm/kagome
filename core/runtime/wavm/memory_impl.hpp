/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

#include "runtime/memory.hpp"

#include <gsl/span>
#include <optional>

#include "common/buffer.hpp"
#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"

namespace kagome::runtime {
  class MemoryAllocator;
}

namespace kagome::runtime::wavm {
  static_assert(kMemoryPageSize == WAVM::IR::numBytesPerPage);

  class MemoryImpl final : public kagome::runtime::Memory {
   public:
    MemoryImpl(WAVM::Runtime::Memory *memory,
               std::unique_ptr<MemoryAllocator> &&allocator);
    MemoryImpl(WAVM::Runtime::Memory *memory, WasmSize heap_base);
    MemoryImpl(const MemoryImpl &copy) = delete;
    MemoryImpl &operator=(const MemoryImpl &copy) = delete;
    MemoryImpl(MemoryImpl &&move) = delete;
    MemoryImpl &operator=(MemoryImpl &&move) = delete;

    WasmPointer allocate(WasmSize size) override;
    std::optional<WasmSize> deallocate(WasmPointer ptr) override;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T load(WasmPointer addr) const {
      auto res = WAVM::Runtime::memoryRef<T>(memory_, addr);
      SL_TRACE_FUNC_CALL(logger_, res, this, addr);
      return res;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T *loadArray(WasmPointer addr, size_t num) const {
      auto res = WAVM::Runtime::memoryArrayPtr<T>(memory_, addr, num);
      SL_TRACE_FUNC_CALL(logger_, gsl::span<T>(res, num), this, addr);
      return res;
    }

    int8_t load8s(WasmPointer addr) const override;
    uint8_t load8u(WasmPointer addr) const override;
    int16_t load16s(WasmPointer addr) const override;
    uint16_t load16u(WasmPointer addr) const override;
    int32_t load32s(WasmPointer addr) const override;
    uint32_t load32u(WasmPointer addr) const override;
    int64_t load64s(WasmPointer addr) const override;
    uint64_t load64u(WasmPointer addr) const override;
    std::array<uint8_t, 16> load128(WasmPointer addr) const override;

    common::BufferView loadN(WasmPointer addr, WasmSize n) const override;

    std::string loadStr(WasmPointer addr, WasmSize n) const override;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void store(WasmPointer addr, T value) {
      SL_TRACE_VOID_FUNC_CALL(logger_, this, addr, value);
      std::memcpy(
          WAVM::Runtime::memoryArrayPtr<uint8_t>(memory_, addr, sizeof(value)),
          &value,
          sizeof(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void storeArray(WasmPointer addr, gsl::span<T> array) {
      SL_TRACE_VOID_FUNC_CALL(logger_, this, addr, array);
      std::memcpy(WAVM::Runtime::memoryArrayPtr<uint8_t>(
                      memory_, addr, sizeof(array.size_bytes())),
                  array.data(),
                  array.size_bytes());
    }

    void store8(WasmPointer addr, int8_t value) override;
    void store16(WasmPointer addr, int16_t value) override;
    void store32(WasmPointer addr, int32_t value) override;
    void store64(WasmPointer addr, int64_t value) override;
    void store128(WasmPointer addr,
                  const std::array<uint8_t, 16> &value) override;
    void storeBuffer(WasmPointer addr, gsl::span<const uint8_t> value) override;

    WasmSpan storeBuffer(gsl::span<const uint8_t> value) override;

    WasmSize size() const override {
      return WAVM::Runtime::getMemoryNumPages(memory_) * kMemoryPageSize;
    }

    void resize(WasmSize new_size) override {
      /**
       * We use this condition to avoid deallocated_ pointers fixup
       */
      if (new_size >= size()) {
        auto new_page_number = (new_size - 1) / kMemoryPageSize + 1;
        WAVM::Runtime::growMemory(
            memory_,
            new_page_number - WAVM::Runtime::getMemoryNumPages(memory_));
      }
    }

   private:
    void fill(PtrSize span, uint8_t value) {
      auto native_ptr =
          WAVM::Runtime::memoryArrayPtr<uint8_t>(memory_, span.ptr, span.size);
      memset(native_ptr, value, span.size);
    }

    std::unique_ptr<MemoryAllocator> allocator_;
    WAVM::Runtime::Memory *memory_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

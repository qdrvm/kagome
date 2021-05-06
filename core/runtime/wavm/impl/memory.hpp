/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

#include <boost/optional.hpp>
#include <gsl/span>

#include "runtime/wavm/impl/crutch.hpp"

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::wavm {

  class Memory {
   public:
    ~Memory() = default;

    explicit Memory(WAVM::Runtime::Memory *memory);

    constexpr static uint32_t kMaxMemorySize =
        std::numeric_limits<uint32_t>::max();
    constexpr static uint8_t kAlignment = 8;

    void setHeapBase(WasmSize heap_base);

    void setUnderlyingMemory(WAVM::Runtime::Memory *memory);

    void reset();

    WasmSize size() const {
      return WAVM::Runtime::getMemoryNumPages(memory_) * kPageSize;
    }

    void resize(WasmSize newSize) {
      auto new_page_number = (newSize / kPageSize) + 1;
      WAVM::Runtime::growMemory(memory_, new_page_number);
    }

    WasmPointer allocate(WasmSize size);
    boost::optional<WasmSize> deallocate(WasmPointer ptr);

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T load(WasmPointer addr) const {
      auto res = WAVM::Runtime::memoryRef<T>(memory_, addr);
      SL_TRACE_FUNC_CALL(logger_, res, addr);
      return res;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T* loadArray(WasmPointer addr, size_t num) const {
      if (addr == 1055850) {
        [](){}();
      }
      auto res = WAVM::Runtime::memoryArrayPtr<T>(memory_, addr, num);
      SL_TRACE_FUNC_CALL(logger_, gsl::span<T>(res, num), addr);
      return res;
    }

    int8_t load8s(WasmPointer addr) const;
    uint8_t load8u(WasmPointer addr) const;
    int16_t load16s(WasmPointer addr) const;
    uint16_t load16u(WasmPointer addr) const;
    int32_t load32s(WasmPointer addr) const;
    uint32_t load32u(WasmPointer addr) const;
    int64_t load64s(WasmPointer addr) const;
    uint64_t load64u(WasmPointer addr) const;
    std::array<uint8_t, 16> load128(WasmPointer addr) const;

    common::Buffer loadN(WasmPointer addr, WasmSize n) const;

    std::string loadStr(WasmPointer addr, WasmSize n) const;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void store(WasmPointer addr, T value) {
      SL_TRACE_VOID_FUNC_CALL(logger_, addr, value);
      std::memcpy(
          WAVM::Runtime::memoryArrayPtr<uint8_t>(memory_, addr, sizeof(value)),
          &value,
          sizeof(value));
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void storeArray(WasmPointer addr, gsl::span<T> array) {
      SL_TRACE_VOID_FUNC_CALL(logger_, addr, array);
      std::memcpy(
          WAVM::Runtime::memoryArrayPtr<uint8_t>(memory_, addr, sizeof(array.size_bytes())),
          array.data(),
          array.size_bytes());
    }

    void store8(WasmPointer addr, int8_t value);
    void store16(WasmPointer addr, int16_t value);
    void store32(WasmPointer addr, int32_t value);
    void store64(WasmPointer addr, int64_t value);
    void store128(WasmPointer addr, const std::array<uint8_t, 16> &value);
    void storeBuffer(WasmPointer addr, gsl::span<const uint8_t> value);

    WasmSpan storeBuffer(gsl::span<const uint8_t> value);

   private:
    constexpr static uint32_t kPageSize = 4096;
    WAVM::Runtime::Memory *memory_;
    WasmPointer offset_;
    WasmPointer heap_base_;
    log::Logger logger_;
    WasmSize size_;

    // map containing addresses of allocated MemoryImpl chunks
    std::unordered_map<WasmPointer, WasmSize> allocated_{};

    // map containing addresses to the deallocated MemoryImpl chunks
    std::unordered_map<WasmPointer, WasmSize> deallocated_{};

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, or -1 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer freealloc(WasmSize size);

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * @param size of target memory
     * @return address of memory of given size, or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer findContaining(WasmSize size);

    /**
     * Resize memory and allocate memory segment of given size
     * @param size memory size to be allocated
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(WasmSize size);

    template <typename T>
    inline T roundUpAlign(T t) {
      return math::roundUp<kAlignment>(t);
    }
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

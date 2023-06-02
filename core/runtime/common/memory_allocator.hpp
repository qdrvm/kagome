/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP

#include <map>
#include <stack>
#include <unordered_map>

#include <optional>

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {

  // Alignment for pointers, same with substrate:
  // https://github.com/paritytech/substrate/blob/743981a083f244a090b40ccfb5ce902199b55334/primitives/allocator/src/freeing_bump.rs#L56
  constexpr uint8_t kAlignment = sizeof(size_t);
  constexpr size_t kDefaultHeapBase = [] {
    using namespace kagome::common::literals;
    return 1_MB;  // 1Mb
  }();

  /**
   * Obtain closest multiple of kAlignment that is greater or equal to given
   * number
   * @tparam T T type of number
   * @param t given number
   * @return closest multiple
   */
  template <typename T>
  static constexpr T roundUpAlign(T t) {
    return math::roundUp<kAlignment>(t);
  }

  inline bool isPowerOf2(size_t x) {
    return ((x > 0ull) && ((x & (x - 1ull)) == 0));
  }

  inline size_t nextHighPowerOf2(size_t k) {
    if (isPowerOf2(k)) {
      return k;
    }
    const auto p = k == 0ull ? 0ull : 64ull - __builtin_clzll(k);
    return (1ull << p);
  }

  /**
   * Implementation of allocator for the runtime memory
   * Combination of monotonic and free-list allocator
   */
  class MemoryAllocator final {
   public:
    struct MemoryHandle {
      std::function<void(size_t)> resize;
      std::function<size_t()> getSize;
      std::function<void(WasmPointer, uint32_t)> storeSz;
      std::function<uint32_t(WasmPointer)> loadSz;
    };
    MemoryAllocator(MemoryHandle memory, WasmPointer heap_base);

    WasmPointer allocate(WasmSize size, bool search_in_deallocates = true);
    std::optional<WasmSize> deallocate(WasmPointer ptr);

    template <typename T>
    bool checkAddress(WasmPointer addr) noexcept {
      return offset_ > addr and offset_ - addr >= sizeof(T);
    }

    bool checkAddress(WasmPointer addr, size_t size) noexcept {
      return offset_ > addr and offset_ - addr >= size;
    }

    /// following methods are needed mostly for testing purposes
    std::optional<WasmSize> getDeallocatedChunkSize(WasmPointer ptr) const;
    std::optional<WasmSize> getAllocatedChunkSize(WasmPointer ptr) const;
    size_t getAllocatedChunksNum() const;
    size_t getDeallocatedChunksNum() const;

   private:
    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, or -1 if it is impossible to
     * allocate this amount of memory
     */

    /**
     * Resize memory and allocate memory segment of given size
     * @param size memory size to be allocated
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(WasmSize size);

    void resize(WasmSize size);

   private:
    MemoryHandle memory_;

    // map containing addresses of allocated MemoryImpl chunks
    // std::unordered_map<WasmPointer, WasmSize> allocated_;

    // map containing addresses to the deallocated MemoryImpl chunks
    // std::map<WasmPointer, WasmSize> deallocated_;
    std::unordered_map<WasmSize, std::stack<WasmPointer>> available_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    size_t offset_;

    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP

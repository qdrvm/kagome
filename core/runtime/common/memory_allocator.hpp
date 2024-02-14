/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <map>
#include <unordered_map>

#include <optional>

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class Memory;

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

    MemoryAllocator(Memory &memory, const struct MemoryConfig &config);

    WasmPointer allocate(const uint32_t size);
    std::optional<WasmSize> deallocate(WasmPointer ptr);

    /*
      Following methods are needed mostly for testing purposes.
      getDeallocatedChunkSize is a slow function with O(N) complexity.
    */
    std::optional<WasmSize> getDeallocatedChunkSize(WasmPointer ptr) const;
    std::optional<WasmSize> getAllocatedChunkSize(WasmPointer ptr) const;
    size_t getDeallocatedChunksNum() const;
    void reset();

   private:
    struct AllocationHeader {
      uint32_t chunk_sz;
      uint32_t allocation_sz;

      void serialize(WasmPointer ptr, MemoryHandle &mh) const {
        mh.storeSz(ptr, chunk_sz);
        mh.storeSz(ptr + sizeof(chunk_sz), allocation_sz);
      }
      void deserialize(WasmPointer ptr, const MemoryHandle &mh) {
        chunk_sz = mh.loadSz(ptr);
        allocation_sz = mh.loadSz(ptr + sizeof(chunk_sz));
      }
    };
    static constexpr size_t AllocationHeaderSz =
        roundUpAlign(sizeof(AllocationHeader));

    /**
     * Resize memory and allocate memory segment of given size
     * @param allocation_sz memory size to be allocated
     * @param chunk_sz is the memory size which is next pow of 2 from
     * alligned(allocation size) + alligned(AllocationHeaderSize)
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(size_t chunk_sz, WasmSize allocation_sz);

    void resize(WasmSize size);

   private:
    MemoryHandle memory_;

    /**
     * Contains information about available chunks for allocation. Key is size
     * and is power of 2.
     */
    std::unordered_map<WasmSize, std::deque<WasmPointer>> available_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    uint32_t offset_;
    uint32_t max_memory_pages_num_;

    log::Logger logger_;
  };

}  // namespace kagome::runtime

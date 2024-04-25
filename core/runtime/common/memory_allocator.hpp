/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "common/literals.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class MemoryHandle;

  // Alignment for pointers, same with substrate:
  // https://github.com/paritytech/substrate/blob/743981a083f244a090b40ccfb5ce902199b55334/primitives/allocator/src/freeing_bump.rs#L56
  constexpr uint8_t kAlignment = 8;
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

  class MemoryAllocator {
   public:
    virtual ~MemoryAllocator() = default;

    virtual WasmPointer allocate(WasmSize size) = 0;
    virtual void deallocate(WasmPointer ptr) = 0;

    /*
      Following methods are needed mostly for testing purposes.
    */
    virtual std::optional<WasmSize> getAllocatedChunkSize(
        WasmPointer ptr) const = 0;
    virtual size_t getDeallocatedChunksNum() const = 0;
  };

  /**
   * Implementation of allocator for the runtime memory
   * Combination of monotonic and free-list allocator
   */
  class MemoryAllocatorImpl final : public MemoryAllocator {
   public:
    MemoryAllocatorImpl(std::shared_ptr<MemoryHandle> memory,
                        const struct MemoryConfig &config);

    virtual WasmPointer allocate(WasmSize size) override;
    virtual void deallocate(WasmPointer ptr) override;

    /*
      Following methods are needed mostly for testing purposes.
    */
    virtual std::optional<WasmSize> getAllocatedChunkSize(
        WasmPointer ptr) const override;
    virtual size_t getDeallocatedChunksNum() const override;

   private:
    using Header = uint64_t;

    // https://github.com/paritytech/polkadot-sdk/blob/polkadot-v1.7.0/substrate/client/allocator/src/freeing_bump.rs#L105
    static constexpr size_t kOrders = 23;
    // https://github.com/paritytech/polkadot-sdk/blob/polkadot-v1.7.0/substrate/client/allocator/src/freeing_bump.rs#L106
    static constexpr WasmSize kMinAllocate = 8;
    static constexpr size_t kMaxAllocate = kMinAllocate << (kOrders - 1);
    static_assert(kMaxAllocate == (32 << 20));
    static constexpr auto kOccupied = uint64_t{1} << 32;
    static constexpr uint32_t kNil = UINT32_MAX;

    uint32_t readOccupied(WasmPointer ptr) const;
    std::optional<uint32_t> readFree(WasmPointer ptr) const;

   private:
    std::shared_ptr<MemoryHandle> memory_;

    std::array<std::optional<uint32_t>, kOrders> free_lists_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    uint32_t offset_;
    uint32_t max_memory_pages_num_;
  };

}  // namespace kagome::runtime

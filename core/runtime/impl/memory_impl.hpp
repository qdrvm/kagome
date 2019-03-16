/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_IMPL_HPP
#define KAGOME_MEMORY_IMPL_HPP

#include <array>
#include <cstring> // for std::memset

#include "runtime/memory.hpp"

namespace kagome::runtime {

  /**
   * Memory implementation for wasm environment
   * Most code is taken from Binaryen's implementation
   */
  class MemoryImpl : public Memory {
   public:
    MemoryImpl();
    explicit MemoryImpl(SizeType size);
    MemoryImpl(MemoryImpl &) = delete;
    MemoryImpl &operator=(const MemoryImpl &) = delete;

    void resize(uint32_t newSize) override;

    std::optional<AddressType> allocate(SizeType size) override;
    std::optional<SizeType> deallocate(AddressType ptr) override;

    int8_t load8s(AddressType addr) override;
    uint8_t load8u(AddressType addr) override;
    int16_t load16s(AddressType addr) override;
    uint16_t load16u(AddressType addr) override;
    int32_t load32s(AddressType addr) override;
    uint32_t load32u(AddressType addr) override;
    int64_t load64s(AddressType addr) override;
    uint64_t load64u(AddressType addr) override;

    std::array<uint8_t, 16> load128(AddressType addr) override;

    void store8(AddressType addr, int8_t value) override;
    void store16(AddressType addr, int16_t value) override;
    void store32(AddressType addr, int32_t value) override;
    void store64(AddressType addr, int64_t value) override;
    void store128(AddressType addr,
                  const std::array<uint8_t, 16> &value) override;

   private:
    // Use char because it doesn't run afoul of aliasing rules.
    std::vector<char> memory_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    AddressType offset_;

    // map containing addresses of allocated MemoryImpl chunks
    std::unordered_map<AddressType, SizeType> allocated;

    // map containing addresses to the deallocated MemoryImpl chunks
    std::unordered_map<AddressType, SizeType> deallocated;

    template <typename T>
    static bool aligned(const char *address) {
      static_assert(!(sizeof(T) & (sizeof(T) - 1)), "must be a power of 2");
      return 0 == (reinterpret_cast<uintptr_t>(address) & (sizeof(T) - 1));
    }

    template <typename T>
    void set(AddressType address, T value) {
      if (aligned<T>(&memory_[address])) {
        *reinterpret_cast<T *>(&memory_[address]) = value;
      } else {
        std::memcpy(&memory_[address], &value, sizeof(T));
      }
    }

    template <typename T>
    T get(AddressType address) const {
      if (aligned<T>(&memory_[address])) {
        return *reinterpret_cast<const T *>(&memory_[address]);
      } else {
        T loaded;
        std::memcpy(&loaded, &memory_[address], sizeof(T));
        return loaded;
      }
    }

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, none if it is impossible to
     * allocate this amount of memory
     */
    std::optional<AddressType> freealloc(SizeType size);

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * @param size of target memory
     * @return address of memory of given size, none if it is impossible to
     * allocate this amount of memory
     */
    std::optional<AddressType> findContaining(SizeType size);
  };

}  // namespace kagome::runtime

#endif  // KAGOME_MEMORY_IMPL_HPP

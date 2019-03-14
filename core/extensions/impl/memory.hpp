/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_HPP
#define KAGOME_MEMORY_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/bind.hpp"

namespace kagome::extensions {

  // The underlying memory can be accessed through unaligned pointers which
  // isn't well-behaved in C++. WebAssembly nonetheless expects it to behave
  // properly. Avoid emitting unaligned load/store by checking for alignment
  // explicitly, and performing memcpy if unaligned.
  //
  // The allocated memory tries to have the same alignment as the memory being
  // simulated.
  class Memory {
   private:
    // Use char because it doesn't run afoul of aliasing rules.
    std::vector<char> memory;

    using Address = size_t;

    // Offset on the tail of the last allocated memory chunk
    Address offset_;

    // map containing addresses of allocated memory chunks
    std::unordered_map<Address, size_t> allocated;

    // map containing addresses to the deallocated memory chunks
    std::unordered_map<Address, size_t> deallocated;

    template <typename T>
    static bool aligned(const char *address) {
      static_assert(!(sizeof(T) & (sizeof(T) - 1)), "must be a power of 2");
      return 0 == (reinterpret_cast<uintptr_t>(address) & (sizeof(T) - 1));
    }
    Memory(Memory &) = delete;
    Memory &operator=(const Memory &) = delete;

   public:
    Memory() {
      offset_ = 0;
    }
    void resize(size_t newSize);

    template <typename T>
    void set(Address address, T value) {
      if (aligned<T>(&memory[address])) {
        *reinterpret_cast<T *>(&memory[address]) = value;
      } else {
        std::memcpy(&memory[address], &value, sizeof(T));
      }
    }
    template <typename T>
    T get(Address address) {
      if (aligned<T>(&memory[address])) {
        return *reinterpret_cast<T *>(&memory[address]);
      } else {
        T loaded;
        std::memcpy(&loaded, &memory[address], sizeof(T));
        return loaded;
      }
    }

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then none is returned
     */
    std::optional<Address> allocate(size_t size);

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    std::optional<size_t> deallocate(Address ptr);

   private:
    /**
     * Finds memory segment of given size within deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, none if it is impossible to
     * allocate this amount of memory
     */
    std::optional<Address> freealloc(size_t size);

    /**
     * Finds memory segment of given size within deallocated pieces of memory
     * @param size of target memory
     * @return address of memory of given size, none if it is impossible to
     * allocate this amount of memory
     */
    std::optional<Address> findContaining(size_t size);
  };
}  // namespace kagome::extensions

#endif  // KAGOME_MEMORY_HPP

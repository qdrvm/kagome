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

namespace kagome::runtime {

  // The underlying memory can be accessed through unaligned pointers which
  // isn't well-behaved in C++. WebAssembly nonetheless expects it to behave
  // properly. Avoid emitting unaligned load/store by checking for alignment
  // explicitly, and performing memcpy if unaligned.
  //
  // The allocated memory tries to have the same alignment as the memory being
  // simulated.
  class Memory {
   public:
    // Address type of wasm memory is 32 bit integer
    using AddressType = uint32_t;
    // Size type is defined by AddressType
    using SizeType = AddressType;

    /**
     * Resizes memory to the given size
     * @param newSize
     */
    virtual void resize(SizeType newSize) = 0;

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then none is returned
     */
    virtual std::optional<AddressType> allocate(SizeType size) = 0;

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    virtual std::optional<SizeType> deallocate(AddressType ptr) = 0;

    /**
     * Load integers from provided address
     */
    virtual int8_t load8s(AddressType addr) = 0;
    virtual uint8_t load8u(AddressType addr) = 0;
    virtual int16_t load16s(AddressType addr) = 0;
    virtual uint16_t load16u(AddressType addr) = 0;
    virtual int32_t load32s(AddressType addr) = 0;
    virtual uint32_t load32u(AddressType addr) = 0;
    virtual int64_t load64s(AddressType addr) = 0;
    virtual uint64_t load64u(AddressType addr) = 0;
    virtual std::array<uint8_t, 16> load128(AddressType addr) = 0;

    /**
     * Store integers at given address of the wasm memory
     */
    virtual void store8(AddressType addr, int8_t value) = 0;
    virtual void store16(AddressType addr, int16_t value) = 0;
    virtual void store32(AddressType addr, int32_t value) = 0;
    virtual void store64(AddressType addr, int64_t value) = 0;
    virtual void store128(AddressType addr,
                          const std::array<uint8_t, 16> &value) = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_MEMORY_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_HPP
#define KAGOME_MEMORY_HPP

#include <array>

#include <boost/optional.hpp>
#include <gsl/span>
#include "common/buffer.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {

  // The underlying memory can be accessed through unaligned pointers which
  // isn't well-behaved in C++. WebAssembly nonetheless expects it to behave
  // properly. Avoid emitting unaligned load/store by checking for alignment
  // explicitly, and performing memcpy if unaligned.
  //
  // The allocated memory tries to have the same alignment as the memory being
  // simulated.
  class WasmMemory {
   public:
    virtual ~WasmMemory() = default;

    constexpr static uint32_t kMaxMemorySize =
        std::numeric_limits<uint32_t>::max();

    /**
     * Resets allocated and deallocated memory information
     */
    virtual void reset() = 0;

    /**
     * @brief Return the size of the memory
     */
    virtual WasmSize size() const = 0;

    /**
     * Resizes memory to the given size
     * @param newSize
     */
    virtual void resize(WasmSize newSize) = 0;

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then -1 is returned
     */
    virtual WasmPointer allocate(WasmSize size) = 0;

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    virtual boost::optional<WasmSize> deallocate(WasmPointer ptr) = 0;

    /**
     * Load integers from provided address
     */
    virtual int8_t load8s(WasmPointer addr) const = 0;
    virtual uint8_t load8u(WasmPointer addr) const = 0;
    virtual int16_t load16s(WasmPointer addr) const = 0;
    virtual uint16_t load16u(WasmPointer addr) const = 0;
    virtual int32_t load32s(WasmPointer addr) const = 0;
    virtual uint32_t load32u(WasmPointer addr) const = 0;
    virtual int64_t load64s(WasmPointer addr) const = 0;
    virtual uint64_t load64u(WasmPointer addr) const = 0;
    virtual std::array<uint8_t, 16> load128(WasmPointer addr) const = 0;

    /**
     * Load bytes from provided address into the buffer of size n
     * @param addr address in memory to load bytes
     * @param n number of bytes to be loaded
     * @return Buffer of length N
     */
    virtual common::Buffer loadN(WasmPointer addr, WasmSize n) const = 0;

    /**
     * Store integers at given address of the wasm memory
     */
    virtual void store8(WasmPointer addr, int8_t value) = 0;
    virtual void store16(WasmPointer addr, int16_t value) = 0;
    virtual void store32(WasmPointer addr, int32_t value) = 0;
    virtual void store64(WasmPointer addr, int64_t value) = 0;
    virtual void store128(WasmPointer addr,
                          const std::array<uint8_t, 16> &value) = 0;
    virtual void storeBuffer(WasmPointer addr,
                             gsl::span<const uint8_t> value) = 0;

    /**
     * @brief allocates buffer in memory and copies value into memory
     * @param value buffer to store
     * @return full wasm pointer to allocated buffer
     */
    virtual WasmSpan storeBuffer(gsl::span<const uint8_t> value) = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_MEMORY_HPP

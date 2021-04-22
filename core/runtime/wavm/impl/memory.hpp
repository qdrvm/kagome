/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

#include <boost/optional.hpp>
#include <gsl/span>

#include "common/buffer.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime::wavm {

  class Memory {
   public:
    
    constexpr static uint32_t kMaxMemorySize =
        std::numeric_limits<uint32_t>::max();

    /**
     * Set heap base to {@param heap_base}
     */
    void setHeapBase(WasmSize heap_base) {}

    /**
     * Resets allocated and deallocated memory information
     */
    void reset() {}

    /**
     * @brief Return the size of the memory
     */
    virtual WasmSize size() const {}

    /**
     * Resizes memory to the given size
     * @param new_size
     */
    virtual void resize(WasmSize new_size) {}

    /**
     * Allocates memory of given size and returns address in the memory
     * @param size allocated memory size
     * @return address to allocated memory. If there is no available slot for
     * such allocation, then -1 is returned
     */
    virtual WasmPointer allocate(WasmSize size) {}

    /**
     * Deallocates memory in provided region
     * @param ptr address of deallocated memory
     * @return size of deallocated memory or none if given address does not
     * point to any allocated pieces of memory
     */
    virtual boost::optional<WasmSize> deallocate(WasmPointer ptr) {}

    /**
     * Load integers from provided address
     */
    virtual int8_t load8s(WasmPointer addr) const {}
    virtual uint8_t load8u(WasmPointer addr) const {}
    virtual int16_t load16s(WasmPointer addr) const {}
    virtual uint16_t load16u(WasmPointer addr) const {}
    virtual int32_t load32s(WasmPointer addr) const {}
    virtual uint32_t load32u(WasmPointer addr) const {}
    virtual int64_t load64s(WasmPointer addr) const {}
    virtual uint64_t load64u(WasmPointer addr) const {}
    virtual std::array<uint8_t, 16> load128(WasmPointer addr) const {}

    /**
     * Load bytes from provided address into the buffer of size n
     * @param addr address in memory to load bytes
     * @param n number of bytes to be loaded
     * @return Buffer of length N
     */
    virtual common::Buffer loadN(WasmPointer addr, WasmSize n) const {}
    /**
     * Load string from address into buffer of size n
     * @param addr address in memory to load bytes
     * @param n number of bytes
     * @return string with data
     */
    virtual std::string loadStr(WasmPointer addr, WasmSize n) const {}

    /**
     * Store integers at given address of the wasm memory
     */
    virtual void store8(WasmPointer addr, int8_t value) {}
    virtual void store16(WasmPointer addr, int16_t value) {}
    virtual void store32(WasmPointer addr, int32_t value) {}
    virtual void store64(WasmPointer addr, int64_t value) {}
    virtual void store128(WasmPointer addr,
                          const std::array<uint8_t, 16> &value) {}
    virtual void storeBuffer(WasmPointer addr,
                             gsl::span<const uint8_t> value) {}

    /**
     * @brief allocates buffer in memory and copies value into memory
     * @param value buffer to store
     * @return full wasm pointer to allocated buffer
     */
    virtual WasmSpan storeBuffer(gsl::span<const uint8_t> value) {}
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MEMORY_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WASM_MEMORY_IMPL_HPP
#define KAGOME_WASM_MEMORY_IMPL_HPP

#include <array>
#include <cstring>  // for std::memset in gcc
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
#include "runtime/wasm_memory.hpp"

namespace kagome::runtime {

  /**
   * Memory implementation for wasm environment
   * Most code is taken from Binaryen's implementation here:
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h#L37
   * @note Memory size of this implementation is at least of the size of one
   * wasm page (4096 bytes)
   */
  class WasmMemoryImpl : public WasmMemory {
   public:
    WasmMemoryImpl();
    explicit WasmMemoryImpl(SizeType size);
    WasmMemoryImpl(const WasmMemoryImpl &copy) = delete;
    WasmMemoryImpl &operator=(const WasmMemoryImpl &copy) = delete;
    WasmMemoryImpl(WasmMemoryImpl &&move) = delete;
    WasmMemoryImpl &operator=(WasmMemoryImpl &&move) = delete;
    ~WasmMemoryImpl() override = default;

    SizeType size() const override;
    void resize(SizeType newSize) override;

    WasmPointer allocate(SizeType size) override;
    boost::optional<SizeType> deallocate(WasmPointer ptr) override;

    int8_t load8s(WasmPointer addr) const override;
    uint8_t load8u(WasmPointer addr) const override;
    int16_t load16s(WasmPointer addr) const override;
    uint16_t load16u(WasmPointer addr) const override;
    int32_t load32s(WasmPointer addr) const override;
    uint32_t load32u(WasmPointer addr) const override;
    int64_t load64s(WasmPointer addr) const override;
    uint64_t load64u(WasmPointer addr) const override;
    std::array<uint8_t, 16> load128(WasmPointer addr) const override;
    common::Buffer loadN(kagome::runtime::WasmPointer addr,
                         kagome::runtime::SizeType n) const override;

    void store8(WasmPointer addr, int8_t value) override;
    void store16(WasmPointer addr, int16_t value) override;
    void store32(WasmPointer addr, int32_t value) override;
    void store64(WasmPointer addr, int64_t value) override;
    void store128(WasmPointer addr,
                  const std::array<uint8_t, 16> &value) override;
    void storeBuffer(kagome::runtime::WasmPointer addr,
                     const kagome::common::Buffer &value) override;

   private:
    // Use char because it doesn't run afoul of aliasing rules.
    std::vector<char> memory_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    WasmPointer offset_;

    // map containing addresses of allocated MemoryImpl chunks
    std::unordered_map<WasmPointer, SizeType> allocated_;

    // map containing addresses to the deallocated MemoryImpl chunks
    std::unordered_map<WasmPointer, SizeType> deallocated_;

    template <typename T>
    static bool aligned(const char *address) {
      static_assert(!(sizeof(T) & (sizeof(T) - 1)), "must be a power of 2");
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return 0 == (reinterpret_cast<uintptr_t>(address) & (sizeof(T) - 1));
    }

    template <typename T>
    void set(WasmPointer address, T value) {
      if (aligned<T>(&memory_[address])) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<T *>(&memory_[address]) = value;
      } else {
        std::memcpy(&memory_[address], &value, sizeof(T));
      }
    }

    template <typename T>
    T get(WasmPointer address) const {
      if (aligned<T>(&memory_[address])) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return *reinterpret_cast<const T *>(&memory_[address]);
      }

      T loaded{};
      std::memcpy(&loaded, &memory_[address], sizeof(T));
      return loaded;
    }

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, or -1 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer freealloc(SizeType size);

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * @param size of target memory
     * @return address of memory of given size, or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer findContaining(SizeType size);

    /**
     * Resize memory and allocate memory segment of given size
     * @param size memory size to be allocated
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(SizeType size);

    void resizeInternal(SizeType newSize);
  };

}  // namespace kagome::runtime

#endif  // KAGOME_WASM_MEMORY_IMPL_HPP

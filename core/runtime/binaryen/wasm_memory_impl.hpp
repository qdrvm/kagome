/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_BINARYEN_WASM_MEMORY_IMPL_HPP
#define KAGOME_RUNTIME_BINARYEN_WASM_MEMORY_IMPL_HPP

#include <binaryen/shell-interface.h>

#include <array>
#include <cstring>  // for std::memset in gcc
#include <memory>
#include <unordered_map>

#include <boost/optional.hpp>

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime::binaryen {

  using namespace kagome::common::literals;

  // Alignment for pointers, same with substrate:
  // https://github.com/paritytech/substrate/blob/743981a083f244a090b40ccfb5ce902199b55334/primitives/allocator/src/freeing_bump.rs#L56
  inline const uint8_t kAlignment = sizeof(size_t);
  inline const size_t kInitialMemorySize = 2_MB;  // 2Mb
  inline const size_t kDefaultHeapBase = 1_MB;    // 1Mb

  /**
   * Obtain closest multiple of kAllignment that is greater or equal to given
   * number
   * @tparam T T type of number
   * @param t given number
   * @return closest multiple
   */
  template <typename T>
  inline constexpr T roundUpAlign(T t) {
    return math::roundUp<kAlignment>(t);
  }

  static_assert(roundUpAlign(kDefaultHeapBase) == kDefaultHeapBase,
                "Heap base must be aligned");
  static_assert(kDefaultHeapBase < kInitialMemorySize,
                "Heap base must be in border of memory");

  /**
   * Memory implementation for wasm environment
   * Most code is taken from Binaryen's implementation here:
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h#L37
   * @note Memory size of this implementation is at least of the size of one
   * wasm page (4096 bytes)
   */
  class WasmMemoryImpl final : public Memory {
   public:
    WasmMemoryImpl(wasm::ShellExternalInterface::Memory *memory,
                            WasmSize heap_base);
    WasmMemoryImpl(const WasmMemoryImpl &copy) = delete;
    WasmMemoryImpl &operator=(const WasmMemoryImpl &copy) = delete;
    WasmMemoryImpl(WasmMemoryImpl &&move) = delete;
    WasmMemoryImpl &operator=(WasmMemoryImpl &&move) = delete;
    ~WasmMemoryImpl() override = default;

    WasmSize size() const override;
    void resize(WasmSize newSize) override;

    WasmPointer allocate(WasmSize size) override;
    boost::optional<WasmSize> deallocate(WasmPointer ptr) override;

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
                         kagome::runtime::WasmSize n) const override;
    std::string loadStr(kagome::runtime::WasmPointer addr,
                        kagome::runtime::WasmSize length) const override;

    void store8(WasmPointer addr, int8_t value) override;
    void store16(WasmPointer addr, int16_t value) override;
    void store32(WasmPointer addr, int32_t value) override;
    void store64(WasmPointer addr, int64_t value) override;
    void store128(WasmPointer addr,
                  const std::array<uint8_t, 16> &value) override;
    void storeBuffer(kagome::runtime::WasmPointer addr,
                     gsl::span<const uint8_t> value) override;

    WasmSpan storeBuffer(gsl::span<const uint8_t> value) override;

    /// following methods are needed mostly for testing purposes
    boost::optional<WasmSize> getDeallocatedChunkSize(WasmPointer ptr) const;
    boost::optional<WasmSize> getAllocatedChunkSize(WasmPointer ptr) const;
    size_t getAllocatedChunksNum() const;
    size_t getDeallocatedChunksNum() const;

   private:
    wasm::ShellExternalInterface::Memory *memory_;
    WasmSize size_;

    // Heap base. Offset is reset to it on reset()
    WasmPointer heap_base_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    WasmPointer offset_;

    log::Logger logger_;

    // map containing addresses of allocated MemoryImpl chunks
    std::unordered_map<WasmPointer, WasmSize> allocated_;

    // map containing addresses to the deallocated MemoryImpl chunks
    std::map<WasmPointer, WasmSize> deallocated_;

    template <typename T>
    static bool aligned(const char *address) {
      static_assert(!(sizeof(T) & (sizeof(T) - 1)), "must be a power of 2");
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return 0 == (reinterpret_cast<uintptr_t>(address) & (sizeof(T) - 1));
    }

    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, or -1 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer freealloc(WasmSize size);

    /**
     * Resize memory and allocate memory segment of given size
     * @param size memory size to be allocated
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(WasmSize size);
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_RUNTIME_BINARYEN_WASM_MEMORY_IMPL_HPP

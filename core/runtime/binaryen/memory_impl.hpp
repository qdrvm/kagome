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
#include <optional>

#include "binaryen/wasm.h"

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {
  class MemoryAllocator;
}

namespace kagome::runtime::binaryen {

  /**
   * Memory implementation for wasm environment
   * Most code is taken from Binaryen's implementation here:
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h#L37
   * @note Memory size of this implementation is at least a page size (4096
   * bytes)
   */
  class MemoryImpl final : public Memory {
   public:
    MemoryImpl(wasm::ShellExternalInterface::Memory *memory,
               std::unique_ptr<MemoryAllocator> &&allocator);
    MemoryImpl(wasm::ShellExternalInterface::Memory *memory,
               WasmSize heap_base);
    MemoryImpl(const MemoryImpl &copy) = delete;
    MemoryImpl &operator=(const MemoryImpl &copy) = delete;
    MemoryImpl(MemoryImpl &&move) = delete;
    MemoryImpl &operator=(MemoryImpl &&move) = delete;
    ~MemoryImpl() override = default;

    void init(wasm::Module const& wasm, wasm::ModuleInstance & instance) {
      for (size_t i = 0; i < size_; i++) {
        memory_->set(i, 0);
      }
      for (auto& segment : wasm.memory.segments) {
        wasm::Address offset = (uint32_t)wasm::ConstantExpressionRunner<wasm::TrivialGlobalManager>(instance.globals).visit(segment.offset).value.geti32();
        if (offset + segment.data.size() > wasm.memory.initial * wasm::Memory::kPageSize) {
          throw std::runtime_error("invalid offset when initializing memory");
        }
        for (size_t i = 0; i != segment.data.size(); ++i) {
          memory_->set(offset + i, segment.data[i]);
        }
      }
    }

    WasmPointer allocate(WasmSize size) override;
    std::optional<WasmSize> deallocate(WasmPointer ptr) override;

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

    void resize(WasmSize new_size) override {
      /**
       * We use this condition to avoid
       * deallocated_ pointers fixup
       */
      if (new_size >= size_) {
        auto old_size = size_;
        size_ = new_size;
        memory_->resize(new_size);

        for(size_t i = old_size; i < size_; i++) {
          memory_->set(i, 0);
        }
      }
    }

    WasmSize size() const override {
      return size_;
    }

    virtual std::string debugDescription() const override {
      return fmt::format("BinaryenMemory{{ internal: {}, size: {}, pageSize: {} }}", fmt::ptr(memory_), size_, kMemoryPageSize);
    }

   private:
    wasm::ShellExternalInterface::Memory *memory_;
    WasmSize size_;
    std::unique_ptr<MemoryAllocator> allocator_;

    log::Logger logger_;
  };
}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_RUNTIME_BINARYEN_WASM_MEMORY_IMPL_HPP

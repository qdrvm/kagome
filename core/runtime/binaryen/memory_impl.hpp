/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <binaryen/shell-interface.h>

#include <array>
#include <cstring>  // for std::memset in gcc
#include <memory>
#include <optional>
#include <unordered_map>

#include <binaryen/wasm.h>

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {
  class MemoryAllocator;
}

namespace kagome::runtime::binaryen {
  static_assert(kMemoryPageSize == wasm::Memory::kPageSize);

  /**
   * Memory implementation for wasm environment
   * Most code is taken from Binaryen's implementation here:
   * https://github.com/WebAssembly/binaryen/blob/master/src/shell-interface.h#L37
   * @note Memory size of this implementation is at least a page size (4096
   * bytes)
   */
  class MemoryImpl final : public MemoryHandle {
   public:
    MemoryImpl(RuntimeExternalInterface::InternalMemory *memory);
    MemoryImpl(const MemoryImpl &copy) = delete;
    MemoryImpl &operator=(const MemoryImpl &copy) = delete;
    MemoryImpl(MemoryImpl &&move) = delete;
    MemoryImpl &operator=(MemoryImpl &&move) = delete;
    ~MemoryImpl() override = default;

    void resize(WasmSize new_size) override {
      /**
       * We use this condition to avoid
       * deallocated_ pointers fixup
       */
      if (new_size >= size()) {
        memory_->pagesResize(sizeToPages(new_size));
      }
    }

    WasmSize size() const override {
      BOOST_ASSERT(memory_ != nullptr);
      return memory_->getSize();
    }

    std::optional<WasmSize> pagesMax() const override;

    outcome::result<BytesOut> view(WasmPointer ptr,
                                   WasmSize size) const override;

   private:
    RuntimeExternalInterface::InternalMemory *memory_;

    log::Logger logger_;
  };
}  // namespace kagome::runtime::binaryen

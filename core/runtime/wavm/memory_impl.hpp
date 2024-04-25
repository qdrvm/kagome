/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/memory.hpp"

#include <optional>
#include <span>

#include "common/buffer.hpp"
#include "common/literals.hpp"
#include "log/logger.hpp"
#include "log/trace_macros.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"

namespace kagome::runtime {
  class MemoryAllocator;
  struct MemoryConfig;
}  // namespace kagome::runtime

namespace kagome::runtime::wavm {
  static_assert(kMemoryPageSize == WAVM::IR::numBytesPerPage);

  class MemoryImpl final : public kagome::runtime::MemoryHandle {
   public:
    MemoryImpl(WAVM::Runtime::Memory *memory, const MemoryConfig &config);
    MemoryImpl(const MemoryImpl &copy) = delete;
    MemoryImpl &operator=(const MemoryImpl &copy) = delete;
    MemoryImpl(MemoryImpl &&move) = delete;
    MemoryImpl &operator=(MemoryImpl &&move) = delete;

    WasmSize size() const override {
      return WAVM::Runtime::getMemoryNumPages(memory_) * kMemoryPageSize;
    }

    std::optional<WasmSize> pagesMax() const override;

    void resize(WasmSize new_size) override {
      /**
       * We use this condition to avoid deallocated_ pointers fixup
       */
      if (new_size >= size()) {
        auto new_page_number = sizeToPages(new_size);
        auto page_num_diff =
            new_page_number - WAVM::Runtime::getMemoryNumPages(memory_);
        WAVM::Runtime::growMemory(memory_, page_num_diff);
      }
    }

    outcome::result<BytesOut> view(WasmPointer ptr,
                                   WasmSize size) const override;

   private:
    WAVM::Runtime::Memory *memory_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime::wavm

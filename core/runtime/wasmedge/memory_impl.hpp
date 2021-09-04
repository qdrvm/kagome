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

struct WasmEdge_MemoryInstanceContext;

namespace kagome::runtime {
  class MemoryAllocator;
}

namespace kagome::runtime::wasmedge {

  class MemoryImpl final : public Memory {
   public:
    MemoryImpl(WasmEdge_MemoryInstanceContext *memory,
               std::unique_ptr<MemoryAllocator>&& allocator);
    MemoryImpl(const MemoryImpl &copy) = delete;
    MemoryImpl &operator=(const MemoryImpl &copy) = delete;
    MemoryImpl(MemoryImpl &&move) = delete;
    MemoryImpl &operator=(MemoryImpl &&move) = delete;
    ~MemoryImpl() override = default;

    WasmPointer allocate(WasmSize size) override;
    boost::optional<WasmSize> deallocate(WasmPointer ptr) override;

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
        size_ = new_size;
        memory_->resize(new_size);
      }
    }

    WasmSize size() const override {
      return size_;
    }

   private:
    WasmEdge_ImportObjectContext *memory_;
    WasmSize size_;
    std::unique_ptr<MemoryAllocator> allocator_;

    log::Logger logger_;
  };
}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_RUNTIME_WASMEDGE_WASM_MEMORY_IMPL_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include "scale/kagome_scale.hpp"

#include "common/buffer.hpp"
#include "common/span_adl.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {
  struct TestMemory {
    TestMemory()
        : m{},
          handle{std::make_shared<TestMemoryHandle>(m)},
          memory{handle, std::make_unique<TestMemoryAllocator>(m)} {}

    mutable common::Buffer m;

    std::shared_ptr<MemoryHandle> handle;
    Memory memory;

    struct TestMemoryHandle : MemoryHandle {
      common::Buffer &m;
      std::optional<WasmSize> pages_max;

      TestMemoryHandle(common::Buffer &m) : m{m} {}

      WasmSize size() const override {
        return m.size();
      }

      std::optional<WasmSize> pagesMax() const override {
        return pages_max;
      }

      void resize(WasmSize size) override {
        m.resize(size);
      }

      outcome::result<BytesOut> view(WasmPointer ptr,
                                     WasmSize size) const override {
        return {reinterpret_cast<uint8_t *>(&m[ptr]), size};
      }
    };

    struct TestMemoryAllocator : MemoryAllocator {
      common::Buffer &m;

      TestMemoryAllocator(common::Buffer &m) : m{m} {}

      WasmPointer allocate(WasmSize size) override {
        auto ptr = m.size();
        m.resize(ptr + size);
        return ptr;
      }

      void deallocate(WasmPointer ptr) override {}
    };

    PtrSize allocate2(WasmSize size) {
      return {memory.allocate(size), size};
    }

    auto operator[](common::BufferView v) {
      return memory.storeBuffer(v);
    }

    auto operator[](WasmPointer) const = delete;
    auto operator[](WasmSpan span) const {
      return SpanAdl{memory.view(span).value()};
    }

    WasmPointer store32u(uint32_t v) {
      auto ptr = memory.allocate(sizeof(v));
      memcpy(memory.view(ptr, sizeof(v)).value().data(), &v, sizeof(v));
      return ptr;
    }

    WasmSpan encode(const auto &v) {
      return memory.storeBuffer(scale::encode(v).value());
    }

    template <typename T>
    T decode(WasmSpan span) const {
      return scale::decode<T>(memory.view(span).value()).value();
    }
  };
}  // namespace kagome::runtime

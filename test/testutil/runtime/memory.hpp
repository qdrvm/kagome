/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

#include "common/span_adl.hpp"
#include "runtime/memory.hpp"

namespace kagome::runtime {
  struct TestMemory : Memory {
    mutable common::Buffer m;

    WasmSize size() const override {
      return m.size();
    }

    void resize(WasmSize size) override {
      m.resize(size);
    }

    outcome::result<BytesOut> view(WasmPointer ptr,
                                   WasmSize size) const override {
      return {reinterpret_cast<uint8_t *>(&m[ptr]), size};
    }

    WasmPointer allocate(WasmSize size) override {
      auto ptr = m.size();
      m.resize(ptr + size);
      return ptr;
    }

    void deallocate(WasmPointer ptr) override {}

    PtrSize allocate2(WasmSize size) {
      return {allocate(size), size};
    }

    auto operator[](common::BufferView v) {
      return storeBuffer(v);
    }

    auto operator[](WasmPointer) const = delete;
    auto operator[](WasmSpan span) const {
      return SpanAdl{Memory::view(span).value()};
    }

    WasmPointer store32u(uint32_t v) {
      auto ptr = allocate(sizeof(v));
      memcpy(view(ptr, sizeof(v)).value().data(), &v, sizeof(v));
      return ptr;
    }

    WasmSpan encode(const auto &v) {
      return storeBuffer(scale::encode(v).value());
    }

    template <typename T>
    T decode(WasmSpan span) const {
      return scale::decode<T>(Memory::view(span).value()).value();
    }
  };
}  // namespace kagome::runtime

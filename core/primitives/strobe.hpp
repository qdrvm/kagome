/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STROBE_HPP
#define KAGOME_STROBE_HPP

#include <cstdint>
#include <array>
#include <cstring>
#include <type_traits>

#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "primitives/math.hpp"

namespace kagome::primitives {

  class Strobe final {
    static constexpr size_t kBufferSize = 200ull;
    static constexpr size_t kAlignment = 8ull;
    static constexpr uint8_t kStrobeR = 166;

    uint8_t raw_data[kBufferSize + kAlignment - 1ull];
    uint8_t *const buffer_;

    template <typename T, size_t kOffset = 0ull>
    constexpr T *as() {
      static_assert(kOffset < count<T>(), "Overflow!");
      return (reinterpret_cast<T *>(buffer_) + kOffset);  // NOLINT
    }

    template <typename T>
    static constexpr size_t count() {
      static_assert(!std::is_reference_v<T>, "Can not be a reference type.");
      static_assert(!std::is_pointer_v<T>, "Can not be a pointer.");
      static_assert(std::is_integral_v<T>, "Cast to non integral type.");
      return kBufferSize / sizeof(T);
    }

    template <size_t kOffset, typename T, size_t N>
    constexpr void load(const T (&data)[N]) { // NOLINT
      static_assert(kOffset + N <= count<T>(), "Load overflows!");
      std::memcpy(as<T, kOffset>(), data, N);
    }

   public:
    Strobe()
        : buffer_{reinterpret_cast<uint8_t *>(
            math::roundUp<kAlignment>(reinterpret_cast<uintptr_t>(raw_data)))} {
    }

    void initialize() {
      std::memset(as<uint8_t>(), 0, count<uint8_t>());
      load<0ull>((uint8_t[]){1, kStrobeR + 2, 1, 0, 1, 96});
      load<6ull>("STROBEv1.0.2");
    }
  };

}

#endif//KAGOME_STROBE_HPP

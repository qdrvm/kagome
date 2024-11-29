/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/twox/twox.hpp"

#include <xxhash.h>

namespace kagome::crypto {

  void make_twox64(const uint8_t *in, uint32_t len, uint8_t *out) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *ptr = reinterpret_cast<uint64_t *>(out);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
  }

  common::Hash64 make_twox64(common::BufferView buf) {
    common::Hash64 hash{};
    make_twox64(buf.data(), buf.size(), hash.data());
    return hash;
  }

  void make_twox128(const uint8_t *in, uint32_t len, uint8_t *out) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *ptr = reinterpret_cast<uint64_t *>(out);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[1] = XXH64(in, len, 1);
  }

  common::Hash128 make_twox128(common::BufferView buf) {
    common::Hash128 hash{};
    make_twox128(buf.data(), buf.size(), hash.data());
    return hash;
  }

  void make_twox256(const uint8_t *in, uint32_t len, uint8_t *out) {
    // Ensure the buffer is aligned to the boundary required for uint64_t
    // (required for happy UBSAN)
    std::array<uint8_t, 4 * sizeof(uint64_t)> aligned_out{};
    // get pointer to the beginning of the aligned buffer
    auto *ptr = reinterpret_cast<uint64_t *>(aligned_out.data());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[1] = XXH64(in, len, 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[2] = XXH64(in, len, 2);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[3] = XXH64(in, len, 3);
    std::memcpy(out, aligned_out.data(), 4 * sizeof(uint64_t));
  }

  common::Hash256 make_twox256(common::BufferView buf) {
    common::Hash256 hash{};
    make_twox256(buf.data(), buf.size(), hash.data());
    return hash;
  }

}  // namespace kagome::crypto

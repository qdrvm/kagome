/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/twox/twox.hpp"

#include <xxhash/xxhash.h>

namespace kagome::crypto {

  void make_twox64(const uint8_t *in, uint32_t len, uint8_t *out) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *ptr = reinterpret_cast<uint64_t *>(out);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
  }

  common::Hash64 make_twox64(std::span<const uint8_t> buf) {
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

  common::Hash128 make_twox128(std::span<const uint8_t> buf) {
    common::Hash128 hash{};
    make_twox128(buf.data(), buf.size(), hash.data());
    return hash;
  }

  void make_twox256(const uint8_t *in, uint32_t len, uint8_t *out) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *ptr = reinterpret_cast<uint64_t *>(out);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[1] = XXH64(in, len, 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[2] = XXH64(in, len, 2);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[3] = XXH64(in, len, 3);
  }

  common::Hash256 make_twox256(std::span<const uint8_t> buf) {
    common::Hash256 hash{};
    make_twox256(buf.data(), buf.size(), hash.data());
    return hash;
  }

}  // namespace kagome::crypto

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

  common::Hash64 make_twox64(gsl::span<const uint8_t> buf) {
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

  common::Hash128 make_twox128(gsl::span<const uint8_t> buf) {
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

  common::Hash256 make_twox256(gsl::span<const uint8_t> buf) {
    common::Hash256 hash{};
    make_twox256(buf.data(), buf.size(), hash.data());
    return hash;
  }

}  // namespace kagome::crypto

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/twox/twox.hpp"

#include <xxhash/xxhash.h>

namespace kagome::crypto {

  Twox128Hash make_twox128(const common::Buffer &buf) {
    return make_twox128(buf.toBytes(), buf.size());
  }

  Twox128Hash make_twox128(const uint8_t *buf, size_t len) {
    Twox128Hash hash{};
    make_twox128(buf, len, hash.data.begin());
    return hash;
  }

  void make_twox128(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox128(in);
    out.putBytes(hash.data.begin(), hash.data.end());
  }

  void make_twox128(const uint8_t *in, uint32_t len, uint8_t *out) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto *ptr = reinterpret_cast<uint64_t *>(out);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[0] = XXH64(in, len, 0);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ptr[1] = XXH64(in, len, 1);
  }

  Twox256Hash make_twox256(const common::Buffer &buf) {
    return make_twox256(buf.toBytes(), buf.size());
  }

  Twox256Hash make_twox256(const uint8_t *buf, size_t len) {
    Twox256Hash hash{};
    make_twox256(buf, len, hash.data.begin());
    return hash;
  }

  void make_twox256(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox256(in);
    out.putBytes(hash.data.begin(), hash.data.end());
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

}  // namespace kagome::crypto

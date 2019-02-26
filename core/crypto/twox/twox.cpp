/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/twox/twox.hpp"

#include <xxhash/xxhash.h>

namespace kagome::crypto {

  Twox128Hash make_twox128(const common::Buffer &buf) {
    return make_twox128(buf.to_bytes(), buf.size());
  }

  Twox128Hash make_twox128(const uint8_t *buf, size_t len) {
    Twox128Hash hash{};
    auto ptr = reinterpret_cast<uint64_t *>(hash.data);  // NOLINT
    ptr[0] = XXH64(buf, len, 0);                         // NOLINT
    ptr[1] = XXH64(buf, len, 1);                         // NOLINT
    return hash;
  }

  void make_twox128(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox128(in);
    out.put_bytes(hash.data, hash.data + sizeof(hash.data));  // NOLINT
  }

  Twox256Hash make_twox256(const common::Buffer &buf) {
    return make_twox256(buf.to_bytes(), buf.size());
  }

  Twox256Hash make_twox256(const uint8_t *buf, size_t len) {
    Twox256Hash hash{};
    auto ptr = reinterpret_cast<uint64_t *>(hash.data);  // NOLINT
    ptr[0] = XXH64(buf, len, 0);                         // NOLINT
    ptr[1] = XXH64(buf, len, 1);                         // NOLINT
    ptr[2] = XXH64(buf, len, 2);                         // NOLINT
    ptr[3] = XXH64(buf, len, 3);                         // NOLINT
    return hash;
  }

  void make_twox256(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox256(in);
    out.put_bytes(hash.data, hash.data + sizeof(hash.data));  // NOLINT
  }

}  // namespace kagome::crypto

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
    Twox128Hash hash;
    uint64_t *ptr = (uint64_t *)hash.data;
    ptr[0] = XXH64(buf, len, 0);
    ptr[1] = XXH64(buf, len, 1);
    return hash;
  }

  void make_twox128(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox128(in);
    for (auto &&data : hash.data) {
      out.put_uint8(data);
    }
  }

  Twox256Hash make_twox256(const common::Buffer &buf) {
    return make_twox256(buf.to_bytes(), buf.size());
  }

  Twox256Hash make_twox256(const uint8_t *buf, size_t len) {
    Twox256Hash hash;
    uint64_t *ptr = (uint64_t *)hash.data;
    ptr[0] = XXH64(buf, len, 0);
    ptr[1] = XXH64(buf, len, 1);
    ptr[2] = XXH64(buf, len, 2);
    ptr[3] = XXH64(buf, len, 3);
    return hash;
  }

  void make_twox256(const common::Buffer &in, common::Buffer &out) {
    auto hash = make_twox256(in);
    for (auto &&data : hash.data) {
      out.put_uint8(data);
    }
  }

}  // namespace kagome::crypto

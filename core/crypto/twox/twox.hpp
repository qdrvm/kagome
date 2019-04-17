/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_TWOX_HPP
#define KAGOME_CRYPTO_TWOX_HPP

#include <array>

#include "common/buffer.hpp"

namespace kagome::crypto {

  struct Twox128Hash {
    std::array<uint8_t, 32> data;
  };
  struct Twox256Hash {
    std::array<uint8_t, 64> data;
  };

  Twox128Hash make_twox128(const common::Buffer &buf);
  Twox128Hash make_twox128(const uint8_t *buf, size_t len);
  void make_twox128(const common::Buffer &in, common::Buffer &out);
  void make_twox128(const uint8_t *in, uint32_t len, uint8_t *out);

  Twox256Hash make_twox256(const common::Buffer &buf);
  Twox256Hash make_twox256(const uint8_t *buf, size_t len);
  void make_twox256(const common::Buffer &in, common::Buffer &out);
  void make_twox256(const uint8_t *in, uint32_t len, uint8_t *out);

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_TWOX_HPP

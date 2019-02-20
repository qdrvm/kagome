/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_TWOX_HPP
#define KAGOME_CRYPTO_TWOX_HPP

#include "common/buffer.hpp"

namespace kagome::crypto {

  struct Twox128Hash {
    uint8_t data[16];
  };
  struct Twox256Hash {
    uint8_t data[32];
  };

  Twox128Hash make_twox128(const common::Buffer &buf);
  Twox128Hash make_twox128(const uint8_t *buf, size_t len);

  Twox256Hash make_twox256(const common::Buffer &buf);
  Twox256Hash make_twox256(const uint8_t *buf, size_t len);

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_TWOX_HPP

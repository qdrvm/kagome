/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_types.hpp"

namespace kagome::crypto {

  SR25519Keypair::SR25519Keypair(gsl::span<uint8_t, SR25519_KEYPAIR_SIZE> kp) {
    std::copy(kp.begin(), kp.begin() + SR25519_SECRET_SIZE, secret_key.begin());
    std::copy(kp.begin() + SR25519_SECRET_SIZE,
              kp.begin() + SR25519_SECRET_SIZE + SR25519_PUBLIC_SIZE,
              public_key.begin());
  }

}  // namespace kagome::crypto

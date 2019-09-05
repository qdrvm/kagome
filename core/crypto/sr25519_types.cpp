/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {

  bool VRFOutput::operator==(const VRFOutput &other) const {
    return value == other.value && proof == other.proof;
  }
  bool VRFOutput::operator!=(const VRFOutput &other) const {
    return !(*this == other);
  }

  SR25519Keypair::SR25519Keypair(gsl::span<uint8_t, SR25519_KEYPAIR_SIZE> kp) {
    BOOST_STATIC_ASSERT(kp.size() == SR25519_KEYPAIR_SIZE);
    std::copy(kp.begin(), kp.begin() + SR25519_SECRET_SIZE, secret_key.begin());
    std::copy(kp.begin() + SR25519_SECRET_SIZE,
              kp.begin() + SR25519_SECRET_SIZE + SR25519_PUBLIC_SIZE,
              public_key.begin());
  }

  bool SR25519Keypair::operator==(const SR25519Keypair &other) const {
    return secret_key == other.secret_key && public_key == other.public_key;
  }

  bool SR25519Keypair::operator!=(const SR25519Keypair &other) const {
    return !(*this == other);
  }

}  // namespace kagome::crypto

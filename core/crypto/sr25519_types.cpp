/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {
  bool Sr25519Keypair::operator==(const Sr25519Keypair &other) const {
    return secret_key == other.secret_key && public_key == other.public_key;
  }

  bool Sr25519Keypair::operator!=(const Sr25519Keypair &other) const {
    return !(*this == other);
  }
}  // namespace kagome::crypto

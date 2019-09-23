/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ed25519_types.hpp"

namespace kagome::crypto {
  bool ED25519Keypair::operator==(const ED25519Keypair &other) const {
    return private_key == other.private_key && public_key == other.public_key;
  }

  bool ED25519Keypair::operator!=(const ED25519Keypair &other) const {
    return !(*this == other);
  }
}  // namespace kagome::crypto

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecdsa_types.hpp"

namespace kagome::crypto {
  bool EcdsaKeypair::operator==(const EcdsaKeypair &other) const {
    return secret_key == other.secret_key && public_key == other.public_key;
  }

  bool EcdsaKeypair::operator!=(const EcdsaKeypair &other) const {
    return !(*this == other);
  }
}  // namespace kagome::crypto

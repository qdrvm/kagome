/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {

  PublicKey::PublicKey(common::KeyType key_type, PublicKey::Buffer &&bytes)
      : Key(key_type, bytes) {}

  PublicKey::PublicKey(common::KeyType key_type, PublicKey::Buffer bytes)
      : Key(key_type, bytes) {}
}  // namespace libp2p::crypto

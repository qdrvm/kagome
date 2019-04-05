/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {

  PrivateKey::PrivateKey(common::KeyType key_type, PrivateKey::Buffer &&bytes)
      : Key(key_type, bytes) {}

  PrivateKey::PrivateKey(common::KeyType key_type, PrivateKey::Buffer bytes)
      : Key(key_type, bytes) {}

}  // namespace libp2p::crypto

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIVATE_KEY_HPP
#define KAGOME_PRIVATE_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  struct PublicKey;
  /**
   * Represents private key
   */
  struct PrivateKey : public Key {};
}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::PrivateKey> {
    size_t operator()(const libp2p::crypto::PrivateKey &x) const {
      return std::hash<libp2p::crypto::Key>()(x);
    }
  };
}  // namespace std

#endif  // KAGOME_PRIVATE_KEY_HPP

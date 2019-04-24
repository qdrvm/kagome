/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PUBLIC_KEY_HPP
#define KAGOME_PUBLIC_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  /**
   * Represents public key
   */
  struct PublicKey : public Key {};
}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::PublicKey> {
    size_t operator()(const libp2p::crypto::PublicKey &x) const {
      return std::hash<libp2p::crypto::Key>()(x);
    }
  };
}  // namespace std

#endif  // KAGOME_PUBLIC_KEY_HPP

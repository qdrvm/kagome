/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_CRYPTO_KEY_HPP
#define KAGOME_LIBP2P_CRYPTO_KEY_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto {

  using kagome::common::Buffer;

  struct Key {
    /**
     * Supported types of all keys
     */
    enum class Type { UNSPECIFIED, RSA1024, RSA2048, RSA4096, ED25519 };
    // TODO(yuraz): add support for Secp256k1 like in js version (added to
    // PRE-103)

    Type type;    ///< key type
    Buffer data;  ///< key content
  };

  inline bool operator==(const Key &a, const Key &b) {
    return a.type == b.type && a.data == b.data;
  }

}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::Key> {
    size_t operator()(const libp2p::crypto::Key &x) const {
      size_t seed = 0;
      boost::hash_combine(seed, x.type);
      boost::hash_combine(seed, std::hash<kagome::common::Buffer>()(x.data));
      return seed;
    }
  };
}  // namespace std

#endif  // KAGOME_LIBP2P_CRYPTO_KEY_HPP

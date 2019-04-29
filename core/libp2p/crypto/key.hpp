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

  struct PublicKey : public Key {};

  struct PrivateKey : public Key {};

  struct KeyPair {
    PublicKey publicKey;
    PrivateKey privateKey;
  };

  inline bool operator==(const KeyPair &a, const KeyPair &b) {
    return a.publicKey == b.publicKey && a.privateKey == b.privateKey;
  }

  /**
   * Result of ephemeral key generation
   */
  struct EphemeralKeyPair {
    kagome::common::Buffer ephemeral_public_key;
    std::function<PrivateKey()> private_key_generator;
  };

  /**
   * Type of the stretched key
   */
  struct StretchedKey {
    kagome::common::Buffer iv;
    kagome::common::Buffer cipher_key;
    kagome::common::Buffer mac_key;
  };

}  // namespace libp2p::crypto

namespace std {
  template <>
  struct hash<libp2p::crypto::Key> {
    size_t operator()(const libp2p::crypto::Key &x) const;
  };

  template <>
  struct hash<libp2p::crypto::PrivateKey> {
    size_t operator()(const libp2p::crypto::PrivateKey &x) const;
  };

  template <>
  struct hash<libp2p::crypto::PublicKey> {
    size_t operator()(const libp2p::crypto::PublicKey &x) const;
  };

  template <>
  struct hash<libp2p::crypto::KeyPair> {
    size_t operator()(const libp2p::crypto::KeyPair &x) const;
  };
}  // namespace std

#endif  // KAGOME_LIBP2P_CRYPTO_KEY_HPP

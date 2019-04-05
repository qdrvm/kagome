/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_COMMON_HPP
#define KAGOME_CRYPTO_COMMON_HPP

#include <cstdint>
#include <memory>

#include "common/buffer.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto::common {
  /**
   * Values for AES-128
   */
  struct Aes128Secret {
    uint8_t key[8];
    uint8_t iv[8];
  };

  /**
   * Values for AES-256
   */
  struct Aes256Secret {
    uint8_t key[16];
    uint8_t iv[8];
  };

  /**
   * Public and private keys
   */
  struct KeyPair {
    std::shared_ptr<PublicKey> public_key;
    std::shared_ptr<PrivateKey> private_key;
  };

  /**
   * Result of ephemeral key generation
   */
  struct EphemeralKeyPair {
    kagome::common::Buffer ephemeral_public_key;
    std::function<PrivateKey()> private_key_generator;
  };

  /**
   * Supported hash types
   */
  enum class HashType { kSHA1, kSHA256, kSHA512 };

  /**
   * Supported types of RSA keys
   */
  enum class RSAKeyType { kRSA1024 = 0, kRSA2048 = 1, kRSA4096 = 2 };

  /**
   * Supported types of all keys
   */
  enum class KeyType : uint32_t { kUnspecified, kRSA1024, kRSA2048, kRSA4096, kED25519 };

  /**
   * Supported ECDH curves
   */
  enum class CurveType { kP256, kP384, kP521 };

  /**
   * Type of the stretched key
   */
  struct StretchedKey {
    kagome::common::Buffer iv;
    kagome::common::Buffer cipher_key;
    kagome::common::Buffer mac_key;
  };

  /**
   * Supported cipher types
   */
  enum class CipherType { kAES128, kAES256 };
}  // namespace libp2p::crypto::common

#endif  // KAGOME_CRYPTO_COMMON_HPP

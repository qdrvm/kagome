/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_COMMON_HPP
#define KAGOME_CRYPTO_COMMON_HPP

#include <cstdint>
#include <functional>
#include <memory>

#include "common/buffer.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto::common {
  /**
   * @struct AesSecret provides key and iv for AES cipher
   * @tparam KeySize key size
   * @tparam IvSize iv size
   */
  template <size_t key_size, size_t iv_size>
  struct AesSecret {
    static constexpr size_t kKeySize = key_size;
    static constexpr size_t kIvSize = iv_size;
    uint8_t key[key_size];
    uint8_t iv[iv_size];
  };

  /**
   * Values for AES-128
   */
  using Aes128Secret = AesSecret<16, 16>;

  /**
   * Values for AES-256
   */
  using Aes256Secret = AesSecret<32, 16>;

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
  enum class RSAKeyType { RSA1024 = 0, RSA2048 = 1, RSA4096 = 2 };

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

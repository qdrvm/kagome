/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_COMMON_HPP
#define KAGOME_CRYPTO_COMMON_HPP

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
   * Supported hash types
   */
  enum class HashType { kSHA1, kSHA256, kSHA512 };

  /**
   * Supported types of keys
   */
  enum class KeyType { kRSA, kED25519 };

  /**
   * Supported ECDH curves
   */
  enum class CurveType { kP256, kP284, kP521 };

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
  enum class CipherType { kAES128, kAES256, kBlowfish };
}  // namespace libp2p::crypto::common

#endif  // KAGOME_CRYPTO_COMMON_HPP

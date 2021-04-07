/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include "outcome/outcome.hpp"

namespace kagome::crypto {

  enum class KeyTypeError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_KEY_TYPE_ID,
  };

  /**
   * @brief Key type identifier
   */
  using KeyTypeId = uint32_t;

  /**
   * Types are 32bit integers, which represent encoded 4-char strings
   * Big-endian byte order is used
   */
  enum KnownKeyTypeId : KeyTypeId {
    KEY_TYPE_BABE = 1650549349u, // BABE, sr25519
    KEY_TYPE_GRAN = 1735549294u, // GRANDPA, ed25519
    KEY_TYPE_ACCO = 1633903471u, // Account control [sr25519, ed25519, secp256k1]
    KEY_TYPE_IMON = 1768779630u, // I'm Online, sr25519
    KEY_TYPE_AUDI = 1635083369u, // Account discovery [sr25519, ed25519, secp256k1]
    KEY_TYPE_LP2P = 1819292272u  // LibP2P
  };

  /**
   * @brief makes string representation of KeyTypeId
   * @param param param key type
   * @return string representation of key type value
   */
  std::string decodeKeyTypeId(KeyTypeId param);

  /**
   * @brief checks whether key type value is supported
   * @param k key type value
   * @return true if supported, false otherwise
   */
  bool isSupportedKeyType(KeyTypeId k);
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);

#endif  // KAGOME_CRYPTO_KEY_TYPE_HPP

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
   * Little-endian byte order is used
   */
  enum KnownKeyTypeId : KeyTypeId {
    // clang-format off
    KEY_TYPE_BABE = 0x62616265u, // BABE, sr25519
    KEY_TYPE_GRAN = 0x6772616eu, // GRANDPA, ed25519
    KEY_TYPE_ACCO = 0x6163636fu, // Account control [sr25519, ed25519, secp256k1]
    KEY_TYPE_IMON = 0x696d6f6eu, // I'm Online, sr25519
    KEY_TYPE_AUDI = 0x61756469u, // Account discovery [sr25519, ed25519, secp256k1]
    KEY_TYPE_LP2P = 0x6c703270u, // LibP2P
    KEY_TYPE_ASGN = 0x6173676eu, // ASGN
    KEY_TYPE_PARA = 0x70617261u, // PARA
    // clang-format on
  };

  /**
   * @brief makes string representation of KeyTypeId
   * @param param param key type
   * @return string representation of key type value
   */
  std::string decodeKeyTypeId(KeyTypeId param);

  KeyTypeId encodeKeyTypeId(std::string str);
  /**
   * @brief checks whether key type value is supported
   * @param k key type value
   * @return true if supported, false otherwise
   */
  bool isSupportedKeyType(KeyTypeId k);
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);

#endif  // KAGOME_CRYPTO_KEY_TYPE_HPP

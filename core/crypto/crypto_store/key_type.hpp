/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include "common/buffer.hpp"
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
    KEY_TYPE_BABE = 0x65626162u, // BABE, sr25519
    KEY_TYPE_GRAN = 0x6e617267u, // GRANDPA, ed25519
    KEY_TYPE_ACCO = 0x6f636361u, // Account control [sr25519, ed25519, secp256k1]
    KEY_TYPE_IMON = 0x6e6f6d69u, // I'm Online, sr25519
    KEY_TYPE_AUDI = 0x69647561u, // Account discovery [sr25519, ed25519, secp256k1]
    KEY_TYPE_ASGN = 0x6e677361u, // ASGN
    KEY_TYPE_PARA = 0x61726170u, // PARA
    KEY_TYPE_BEEF = 0x66656562u, // Beefy, secp256k1
    // clang-format on
  };

  /**
   * @brief makes string representation of KeyTypeId
   * @param key_type_id KeyTypeId
   * @return string representation of KeyTypeId
   */
  std::string encodeKeyTypeIdToStr(KeyTypeId key_type_id);

  /**
   * @brief restores KeyTypeId from its string representation
   * @param param string representation of key type
   * @return KeyTypeId
   */
  KeyTypeId decodeKeyTypeIdFromStr(std::string_view str);

  /**
   * @brief checks whether key type value is supported
   * @param k key type value
   * @return true if supported, false otherwise
   */
  bool isSupportedKeyType(KeyTypeId k);

  std::string encodeKeyFileName(KeyTypeId type, common::BufferView key);

  outcome::result<std::pair<KeyTypeId, common::Buffer>> decodeKeyFileName(
      std::string_view name);
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);

#endif  // KAGOME_CRYPTO_KEY_TYPE_HPP

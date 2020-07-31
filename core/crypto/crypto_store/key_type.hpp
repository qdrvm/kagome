/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include "common/blob.hpp"
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

  namespace key_types {
    /**
     * Types are 32bit integers, which represent encoded 4-char strings
     * Big-endian byte order is used
     */
    static constexpr KeyTypeId kBabe = 1650549349u;  // "babe"
    static constexpr KeyTypeId kGran = 1735549294u;  // "gran"
    static constexpr KeyTypeId kAcco = 1633903471u;  // "acco"
    static constexpr KeyTypeId kImon = 1768779630u;  // "imon"
    static constexpr KeyTypeId kAudi = 1635083369u;  // "audi"
    static constexpr KeyTypeId kLp2p = 1819292272u;  // "lp2p"
  }                                                  // namespace key_types

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

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include <libp2p/crypto/key.hpp>
#include "common/blob.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {
  using libp2p::crypto::KeyPair;
  using libp2p::crypto::PrivateKey;
  using libp2p::crypto::PublicKey;

  enum class KeyTypeError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_KEY_TYPE_ID,
  };

  enum class SupportedKeyTypes : uint32_t {
    BABE = 'babe',
    GRAN = 'gran',
    ACCO = 'acco',
    IMON = 'imon',
    AUDI = 'audi'
  };

  /**
   * @brief Key type identifier
   */
  using KeyTypeId = uint32_t;

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

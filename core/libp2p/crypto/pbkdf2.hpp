/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PBKDF2_HPP
#define KAGOME_PBKDF2_HPP

#include <string>
#include <string_view>

#include "libp2p/crypto/crypto_common.hpp"

namespace libp2p::crypto {
  /**
   * Get a secure hash of the password
   * @param password to be hashed
   * @param salt to be used in hashing process
   * @param iterations - number of iterations
   * @param key_size - size of the key in bytes
   * @param hash - hashing algorithm
   * @return a new password
   */
  std::string pbkdf2(std::string_view password, std::string_view salt,
                     uint8_t iterations, size_t key_size,
                     common::HashType hash);
}  // namespace libp2p::crypto

#endif  // KAGOME_PBKDF2_HPP

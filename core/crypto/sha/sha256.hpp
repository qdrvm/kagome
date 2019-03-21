/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SHA256_HPP
#define KAGOME_SHA256_HPP

#include <string_view>

#include "common/buffer.hpp"

namespace kagome::crypto {
  /**
   * SHA-256 hash a string
   * @param str to be hashed
   * @return hashed string in bytes format
   */
  common::Buffer sha256(std::string_view str);

  /**
   * SHA-256 hash the bytes
   * @param bytes to be hashed
   * @return hashed bytes
   */
  common::Buffer sha256(const common::Buffer &bytes);
}  // namespace kagome::crypto

#endif  // KAGOME_SHA256_HPP

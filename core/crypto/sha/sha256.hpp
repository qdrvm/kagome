/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SHA256_HPP
#define KAGOME_SHA256_HPP

#include <string_view>

#include <gsl/span>
#include "common/blob.hpp"

namespace kagome::crypto {
  /**
   * Take a SHA-256 hash from string
   * @param input to be hashed
   * @return hashed bytes
   */
  common::Hash256 sha256(std::string_view input);

  /**
   * Take a SHA-256 hash from bytes
   * @param input to be hashed
   * @return hashed bytes
   */
  common::Hash256 sha256(gsl::span<const uint8_t> input);
}  // namespace kagome::crypto

#endif  // KAGOME_SHA256_HPP

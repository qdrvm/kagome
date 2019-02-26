/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HEXUTIL_HPP
#define KAGOME_HEXUTIL_HPP

#include <vector>

#include "common/result.hpp"
#include "common/unhex_errors.hpp"

namespace kagome::common {

  /**
   * @brief Converts bytes to hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   *
   * @note produces uppercase hexstrings
   */
  std::string hex(const uint8_t *array, size_t len) noexcept;
  std::string hex(const std::vector<uint8_t> &bytes) noexcept;

  /**
   * @brief Converts hex representation to bytes
   * @param array individual chars
   * @param len length of chars
   * @return Result containing array of bytes if input string is hex encoded and
   * has even length. Otherwise Result containing error message is returned
   *
   * @note reads both uppercase and lowercase hexstrings
   *
   * @see
   * https://www.boost.org/doc/libs/1_51_0/libs/algorithm/doc/html/the_boost_algorithm_library/Misc/hex.html
   */
  expected::Result<std::vector<uint8_t>, UnhexErrors> unhex(
      std::string_view hex);

}  // namespace kagome::common

#endif  // KAGOME_HEXUTIL_HPP

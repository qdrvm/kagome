/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HEXUTIL_HPP
#define KAGOME_HEXUTIL_HPP

#include <string>
#include <vector>

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
   * @return array of bytes
   * @throws boost::algorithm::not_enough_input if input has odd length.
   * @throws boost::algorithm::non_hex_input if input contains non-hex symbol.
   *
   * @note catch boost::algorithm::hex_decode_error to catch all errors
   * @note reads both uppercase and lowercase hexstrings
   *
   * @see
   * https://www.boost.org/doc/libs/1_51_0/libs/algorithm/doc/html/the_boost_algorithm_library/Misc/hex.html
   */
  std::vector<uint8_t> unhex(const std::string &hex);

}  // namespace kagome::common

#endif  // KAGOME_HEXUTIL_HPP

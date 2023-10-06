/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>
#include <vector>

#include <gsl/span>
#include "outcome/outcome.hpp"

namespace kagome::common {

  /**
   * @brief error codes for exceptions that may occur during unhexing
   */
  enum class UnhexError {
    NOT_ENOUGH_INPUT = 1,
    NON_HEX_INPUT,
    VALUE_OUT_OF_RANGE,
    MISSING_0X_PREFIX,
    UNKNOWN
  };

  /**
   * @brief Converts an integer to an uppercase hex representation
   */
  std::string int_to_hex(uint64_t n, size_t fixed_width = 2) noexcept;

  /**
   * @brief Converts bytes to uppercase hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_upper(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts bytes to hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_lower(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Converts bytes to hex representation with prefix 0x
   * @param array bytes
   * @return hexstring
   */
  std::string hex_lower_0x(gsl::span<const uint8_t> bytes) noexcept;

  /**
   * @brief Adapter for ptr+size
   */
  inline std::string hex_lower_0x(const uint8_t *data, size_t size) noexcept {
    return hex_lower_0x(gsl::span<const uint8_t>(data, size));
  }

  /**
   * @brief Converts hex representation to bytes
   * @param array individual chars
   * @param len length of chars
   * @return result containing array of bytes if input string is hex encoded and
   * has even length
   *
   * @note reads both uppercase and lowercase hexstrings
   *
   * @see
   * https://www.boost.org/doc/libs/1_51_0/libs/algorithm/doc/html/the_boost_algorithm_library/Misc/hex.html
   */
  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex);

  /**
   * @brief Unhex hex-string with 0x in the begining
   * @param hex hex string with 0x in the beginning
   * @return unhexed buffer
   */
  outcome::result<std::vector<uint8_t>> unhexWith0x(std::string_view hex);

  /**
   * @brief unhex hex-string with 0x or without it in the beginning
   * @tparam T unsigned integer value type to decode
   * @param value source hex string
   * @return unhexed value
   */
  template <class T, typename = std::enable_if<std::is_unsigned_v<T>>>
  outcome::result<T> unhexNumber(std::string_view value) {
    std::vector<uint8_t> bytes;
    OUTCOME_TRY(bts, common::unhexWith0x(value));
    bytes = std::move(bts);

    if (bytes.size() > sizeof(T)) {
      return UnhexError::VALUE_OUT_OF_RANGE;
    }

    T result{0u};
    for (auto b : bytes) {
      // check if `multiply by 10` will cause overflow
      if constexpr (sizeof(T) > 1) {
        result <<= 8u;
      } else {
        result = 0;
      }
      result += b;
    }

    return result;
  }

}  // namespace kagome::common

OUTCOME_HPP_DECLARE_ERROR(kagome::common, UnhexError);

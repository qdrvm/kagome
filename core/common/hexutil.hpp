/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/algorithm/hex.hpp>
#include <string_view>
#include <vector>

#include "outcome/outcome.hpp"

namespace kagome::common {

  class BufferView;

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
}  // namespace kagome::common

OUTCOME_HPP_DECLARE_ERROR(kagome::common, UnhexError);

namespace kagome::common {
  /**
   * @brief Converts bytes to hex representation
   * @param array bytes
   * @param len length of bytes
   * @return hexstring
   */
  std::string hex_lower(BufferView bytes);

  /**
   * @brief Converts bytes to hex representation with prefix 0x
   * @param array bytes
   * @return hexstring
   */
  std::string hex_lower_0x(BufferView bytes);

  template <std::output_iterator<uint8_t> Iter>
  outcome::result<void> unhex_to(std::string_view hex, Iter out) {
    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), out);
      return outcome::success();

    } catch (const boost::algorithm::not_enough_input &e) {
      return UnhexError::NOT_ENOUGH_INPUT;

    } catch (const boost::algorithm::non_hex_input &e) {
      return UnhexError::NON_HEX_INPUT;

    } catch (const std::exception &e) {
      return UnhexError::UNKNOWN;
    }
  }

  template <std::output_iterator<uint8_t> Iter>
  outcome::result<void> unhexWith0x(std::string_view hex_with_prefix,
                                    Iter out) {
    constexpr std::string_view prefix = "0x";
    if (!hex_with_prefix.starts_with(prefix)) {
      return UnhexError::MISSING_0X_PREFIX;
    }
    return common::unhex_to(hex_with_prefix.substr(prefix.size()), out);
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
   * @brief Unhex hex-string with 0x in the beginning
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

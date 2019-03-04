/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>

namespace kagome::common {

  template std::string hex<true>(const uint8_t *array, size_t len) noexcept;
  template std::string hex<false>(const uint8_t *array, size_t len) noexcept;
  template std::string hex<true>(const std::vector<uint8_t> &bytes) noexcept;
  template std::string hex<false>(const std::vector<uint8_t> &bytes) noexcept;

  template <bool IsUpper>
  std::string hex(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    if (IsUpper) {
      boost::algorithm::hex(array, array + len, res.begin());  // NOLINT
    } else {
      boost::algorithm::hex_lower(array, array + len, res.begin());  // NOLINT
    }
    return res;
  }

  template <bool IsUpper>
  std::string hex(const std::vector<uint8_t> &bytes) noexcept {
    return hex<IsUpper>(bytes.data(), bytes.size());
  }

  expected::Result<std::vector<uint8_t>, UnhexError> unhex(
      std::string_view hex) {
    std::vector<uint8_t> blob((hex.size() + 1) / 2);
    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), blob.begin());
      return expected::Value{blob};
    } catch (const boost::algorithm::not_enough_input &e) {
      return expected::Error{UnhexError::kNotEnoughInput};
    } catch (const boost::algorithm::non_hex_input &e) {
      return expected::Error{UnhexError::kNonHexInput};
    };
  }
}  // namespace kagome::common

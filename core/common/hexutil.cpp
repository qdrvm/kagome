/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

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

  expected::Result<std::vector<uint8_t>, std::string> unhex(
      std::string_view hex) {
    std::vector<uint8_t> blob((hex.size() + 1) / 2);

    boost::format
        error_format;  // for storing formatted error message if any occurs
    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), blob.begin());
      return expected::Value{blob};
    } catch (const std::exception &e) {
      return expected::Error{e.what()};
    }
  }
}  // namespace kagome::common

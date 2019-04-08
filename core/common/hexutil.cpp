/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

namespace kagome::common {

  std::string int_to_hex(uint64_t n) noexcept {
    std::stringstream result;
    result.width(2);
    result.fill('0');
    result << std::hex << std::uppercase << n;
    auto str = result.str();
    if (str.length() % 2 != 0) {
      str.push_back('\0');
      for (int i = str.length() - 2; i >= 0; --i) {
        str[i + 1] = str[i];
      }
      str[0] = '0';
    }
    return str;
  }

  std::string hex_upper(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    boost::algorithm::hex(array, array + len, res.begin());  // NOLINT
    return res;
  }

  std::string hex_upper(const std::vector<uint8_t> &bytes) noexcept {
    return hex_upper(bytes.data(), bytes.size());
  }

  std::string hex_lower(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    boost::algorithm::hex_lower(array, array + len, res.begin());  // NOLINT
    return res;
  }

  std::string hex_lower(const std::vector<uint8_t> &bytes) noexcept {
    return hex_lower(bytes.data(), bytes.size());
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

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

namespace kagome::common {

  std::string hex(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    boost::algorithm::hex(array, array + len, res.begin());  // NOLINT
    return res;
  }

  std::string hex(const std::vector<uint8_t> &bytes) noexcept {
    return hex(bytes.data(), bytes.size());
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

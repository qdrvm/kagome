/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <boost/algorithm/hex.hpp>

#include "hexutil.hpp"

namespace kagome::common {

  std::string hex(const uint8_t *array, size_t len) noexcept {
    std::string res(len * 2, '\x00');
    boost::algorithm::hex(array, array + len, res.begin()); // NOLINT
    return res;
  }

  std::string hex(const std::vector<uint8_t> &bytes) noexcept {
    return hex(bytes.data(), bytes.size());
  }

  std::vector<uint8_t> unhex(const std::string &hex) {
    std::vector<uint8_t> blob((hex.size() + 1) / 2);
    boost::algorithm::unhex(hex.begin(), hex.end(), blob.begin());
    return blob;
  }

}  // namespace kagome::common

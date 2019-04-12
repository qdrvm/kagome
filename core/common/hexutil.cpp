/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::common, UnhexError, e) {
  using kagome::common::UnhexError;
  switch (e) {
    case UnhexError::NON_HEX_INPUT:
      return "Input contains non-hex characters";
    case UnhexError::NOT_ENOUGH_INPUT:
      return "Input contains odd number of characters";
    default:
      return "Unknown error";
  }
}

namespace kagome::common {

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

  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex) {
    std::vector<uint8_t> blob((hex.size() + 1) / 2);

    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), blob.begin());
      return blob;

    } catch (const boost::algorithm::not_enough_input &e) {
      return UnhexError::NOT_ENOUGH_INPUT;

    } catch (const boost::algorithm::non_hex_input &e) {
      return UnhexError::NON_HEX_INPUT;

    } catch (const std::exception &e) {
      return UnhexError::UNKNOWN;
    }
  }
}  // namespace kagome::common

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>

#include "common/buffer_view.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::common, UnhexError, e) {
  using kagome::common::UnhexError;
  switch (e) {
    case UnhexError::NON_HEX_INPUT:
      return "Input contains non-hex characters";
    case UnhexError::NOT_ENOUGH_INPUT:
      return "Input contains odd number of characters";
    case UnhexError::VALUE_OUT_OF_RANGE:
      return "Decoded value is out of range of requested type";
    case UnhexError::MISSING_0X_PREFIX:
      return "Missing expected 0x prefix";
    case UnhexError::UNKNOWN:
      return "Unknown error";
  }
  return "Unknown error (error id not listed)";
}

namespace kagome::common {

  std::string int_to_hex(uint64_t n, size_t fixed_width) noexcept {
    std::stringstream result;
    result.width(fixed_width);
    result.fill('0');
    result << std::hex << std::uppercase << n;
    auto str = result.str();
    if (str.length() % 2 != 0) {
      str.push_back('\0');
      for (int64_t i = str.length() - 2; i >= 0; --i) {
        str[i + 1] = str[i];
      }
      str[0] = '0';
    }
    return str;
  }

  std::string hex_upper(BufferView bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex(bytes.begin(), bytes.end(), res.begin());
    return res;
  }

  std::string hex_lower(BufferView bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex_lower(bytes.begin(), bytes.end(), res.begin());
    return res;
  }

  std::string hex_lower_0x(BufferView bytes) noexcept {
    constexpr char prefix[] = {'0', 'x'};
    constexpr size_t prefix_len = sizeof(prefix);

    std::string res(bytes.size() * 2 + prefix_len, '\x00');
    res.replace(0, prefix_len, prefix, prefix_len);

    boost::algorithm::hex_lower(
        bytes.begin(), bytes.end(), res.begin() + prefix_len);
    return res;
  }

  std::string hex_lower_0x(const uint8_t *data, size_t size) noexcept {
    return hex_lower_0x(BufferView(data, size));
  }

  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex) {
    std::vector<uint8_t> blob;
    blob.reserve((hex.size() + 1) / 2);

    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), std::back_inserter(blob));
      return blob;

    } catch (const boost::algorithm::not_enough_input &e) {
      return UnhexError::NOT_ENOUGH_INPUT;

    } catch (const boost::algorithm::non_hex_input &e) {
      return UnhexError::NON_HEX_INPUT;

    } catch (const std::exception &e) {
      return UnhexError::UNKNOWN;
    }
  }

  outcome::result<std::vector<uint8_t>> unhexWith0x(
      std::string_view hex_with_prefix) {
    static const std::string leading_chrs = "0x";

    if (hex_with_prefix.substr(0, leading_chrs.size()) != leading_chrs) {
      return UnhexError::MISSING_0X_PREFIX;
    }

    auto without_prefix = hex_with_prefix.substr(leading_chrs.size());

    return common::unhex(without_prefix);
  }
}  // namespace kagome::common

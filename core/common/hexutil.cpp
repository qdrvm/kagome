/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include <gsl/span>

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

  std::string hex_upper(const gsl::span<const uint8_t> bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex(bytes.begin(), bytes.end(), res.begin());
    return res;
  }

  std::string hex_lower(const gsl::span<const uint8_t> bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex_lower(bytes.begin(), bytes.end(), res.begin());
    return res;
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
    const static std::string leading_chrs = "0x";

    auto without_prefix = hex_with_prefix.substr(
        hex_with_prefix.find(leading_chrs) + leading_chrs.length(),
        hex_with_prefix.length() - 1);
    return common::unhex(without_prefix);
  }
}  // namespace kagome::common

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multibase_codec/codecs/base16.hpp"

#include <algorithm>
#include <cctype>

#include "common/hexutil.hpp"

namespace {
  /**
   * Check, if hex string is in uppercase
   * @param string to be checked
   * @return true, if provided string is uppercase hex-encoded, false otherwise
   */
  bool encodingCaseIsUpper(std::string_view string) {
    return std::all_of(string.begin(), string.end(), [](const char &c) {
      return !std::isalpha(c) || static_cast<bool>(std::isupper(c));
    });
  }
}  // namespace

namespace libp2p::multi::detail {
  using namespace kagome::common;
  using namespace kagome::expected;

  std::string encodeBase16Upper(const Buffer &bytes) {
    return hex_upper(bytes.toVector());
  }

  std::string encodeBase16Lower(const Buffer &bytes) {
    return hex_lower(bytes.toVector());
  }

  Result<Buffer, std::string> decodeBase16Upper(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (!encodingCaseIsUpper(string)) {
      return Error{"cannot unhex string '" + std::string{string}
                   + "': input is not in the uppercase hex"};
    }
    return unhex(string) | [](auto &&v) -> Result<Buffer, std::string> {
      return Value{Buffer{std::forward<decltype(v)>(v)}};
    };
  }

  Result<Buffer, std::string> decodeBase16Lower(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (encodingCaseIsUpper(string)) {
      return Error{"cannot unhex string '" + std::string{string}
                   + "': input is not in the lowercase hex"};
    }
    return unhex(string) | [](auto &&v) -> Result<Buffer, std::string> {
      return Value{Buffer{std::forward<decltype(v)>(v)}};
    };
  }

}  // namespace libp2p::multi::detail

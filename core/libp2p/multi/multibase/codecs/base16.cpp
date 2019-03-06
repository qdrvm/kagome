/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>
#include <cctype>

#include "common/hexutil.hpp"
#include "libp2p/multi/multibase/codecs/base16.hpp"

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

namespace libp2p::multi {
  using namespace kagome::common;
  using namespace kagome::expected;

  template std::string Base16Codec<true>::encode(const Buffer &bytes) const;
  template std::string Base16Codec<false>::encode(const Buffer &bytes) const;
  template Result<Buffer, std::string> Base16Codec<true>::decode(
      std::string_view string) const;
  template Result<Buffer, std::string> Base16Codec<false>::decode(
      std::string_view string) const;

  template <bool IsUpper>
  std::string Base16Codec<IsUpper>::encode(const Buffer &bytes) const {
    return hex<IsUpper>(bytes.toVector());
  }

  template <bool IsUpper>
  Result<Buffer, std::string> Base16Codec<IsUpper>::decode(
      std::string_view string) const {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if ((IsUpper && !encodingCaseIsUpper(string))
        || (!IsUpper && encodingCaseIsUpper(string))) {
      return Error{"could not unhex string '" + std::string{string}
                   + "': input is not in the provided hex case"};
    }
    return unhex(string) |
               [](std::vector<uint8_t> v) -> Result<Buffer, std::string> {
      return Value{Buffer{v}};
    };
  }
}  // namespace libp2p::multi

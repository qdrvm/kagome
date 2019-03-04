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
   * Check, if hex string is actually in upper or lowercase
   * @param string to be checked
   * @param encoding_is_upper - true, if string is to be in uppercase, false
   * otherwise
   * @return result of the check
   */
  bool encodingCaseCorrect(std::string_view string, bool encoding_is_upper) {
    return std::all_of(
        string.begin(), string.end(), [encoding_is_upper](const char &c) {
          if (not std::isalpha(c)) {
            return true;
          }
          if (encoding_is_upper) {
            return static_cast<bool>(std::isupper(c));
          }
          return static_cast<bool>(std::islower(c));
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
    if (!encodingCaseCorrect(string, IsUpper)) {
      return Error{"could not unhex string '" + std::string{string}
                   + "': input is not in the provided hex case"};
    }
    return unhex(string).match(
        [](const Value<std::vector<uint8_t>> &v)
            -> Result<Buffer, std::string> { return Value{Buffer{v.value}}; },
        [string](Error<UnhexError> err) -> Result<Buffer, std::string> {
          switch (err.error) {
            case UnhexError::kNotEnoughInput:
              return Error{"could not unhex string '" + std::string{string}
                           + "': input is either empty or contains odd number "
                             "of symbols"};
            case UnhexError::kNonHexInput:
              return Error{"could not unhex string '" + std::string{string}
                           + "': input is not hex-encoded"};
          }
        });
  }
}  // namespace libp2p::multi

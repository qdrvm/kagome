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
      return !std::isalpha(c) || static_cast<bool>(std::isupper(c));  // NOLINT
    });
  }
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::detail, Base16DecodeError, e) {
  switch (e) {
    case libp2p::multi::detail::Base16DecodeError::kNonUppercaseInput:
      return "Input is not in the uppercase hex";
    case libp2p::multi::detail::Base16DecodeError::kNonLowercaseInput:
      return "Input is not in the lowercase hex";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi::detail {

  using kagome::common::Buffer;
  using kagome::common::hex_lower;
  using kagome::common::hex_upper;
  using kagome::common::unhex;

  std::string encodeBase16Upper(const Buffer &bytes) {
    return hex_upper(bytes.toVector());
  }

  std::string encodeBase16Lower(const Buffer &bytes) {
    return hex_lower(bytes.toVector());
  }

  outcome::result<Buffer> decodeBase16Upper(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (!encodingCaseIsUpper(string)) {
      return Base16DecodeError::kNonUppercaseInput;
    }
    OUTCOME_TRY(bytes, unhex(string));
    return Buffer{std::move(bytes)};
  }

  outcome::result<Buffer> decodeBase16Lower(std::string_view string) {
    // we need this check, because Boost can unhex any kind of base16 with one
    // func, but the base must be specified correctly
    if (encodingCaseIsUpper(string)) {
      return Base16DecodeError::kNonLowercaseInput;
    }
    OUTCOME_TRY(bytes, unhex(string));
    return Buffer{std::move(bytes)};
  }

}  // namespace libp2p::multi::detail

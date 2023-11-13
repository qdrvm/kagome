/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <qtils/hex.hpp>
#include <qtils/unhex.hpp>

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
  std::string hex_lower(qtils::BytesIn bytes) noexcept {
    return fmt::format("{:x}", bytes);
  }

  std::string hex_lower_0x(qtils::BytesIn bytes) noexcept {
    return fmt::format("{:0x}", bytes);
  }

  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex) {
    return qtils::unhex(hex, qtils::Unhex0x::No);
  }

  outcome::result<std::vector<uint8_t>> unhexWith0x(std::string_view hex) {
    return qtils::unhex(hex, qtils::Unhex0x::Yes);
  }
}  // namespace kagome::common

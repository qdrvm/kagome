/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_TOKEN_ERROR_HPP
#define KAGOME_CORE_PRIMITIVES_TOKEN_ERROR_HPP

#include <cstdint>

#include "outcome/outcome.hpp"

namespace kagome::primitives {

  enum class TokenError : uint8_t {
    /// Funds are unavailable.
    NoFunds = 1,
    /// Account that must exist would die.
    WouldDie,
    /// Account cannot exist with the funds that would be given.
    BelowMinimum,
    /// Account cannot be created.
    CannotCreate,
    /// The asset in question is unknown.
    UnknownAsset,
    /// Funds exist but are frozen.
    Frozen,
    /// Operation is not supported by the asset.
    Unsupported,
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const TokenError &v) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    return s << static_cast<uint8_t>(v) - 1;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, TokenError &v) {
    uint8_t value = 0u;
    s >> value;
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    ++value;
    v = static_cast<TokenError>(value);
    return s;
  }

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, TokenError);

#endif  // KAGOME_CORE_PRIMITIVES_TOKEN_ERROR_HPP

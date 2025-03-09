/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include <scale/scale.hpp>

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

  inline void encode(const TokenError &v, scale::Encoder &encoder) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    encoder.put(static_cast<uint8_t>(v) - 1);
  }

  inline void decode(TokenError &v, scale::Decoder &decoder) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    v = static_cast<TokenError>(decoder.take() + 1);
  }

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, TokenError);

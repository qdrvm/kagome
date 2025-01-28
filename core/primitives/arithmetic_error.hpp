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

  enum class ArithmeticError : uint8_t {
    /// Underflow.
    Underflow = 1,
    /// Overflow.
    Overflow,
    /// Division by zero.
    DivisionByZero,
  };

  inline scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                               const ArithmeticError &v) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    return s << (static_cast<uint8_t>(v) - 1);
  }

  inline scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                               ArithmeticError &v) {
    uint8_t value = 0u;
    s >> value;
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    ++value;
    v = static_cast<ArithmeticError>(value);
    return s;
  }

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, ArithmeticError);

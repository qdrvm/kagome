/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
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

  inline void encode(const ArithmeticError &v, scale::Encoder &encoder) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    encoder.put(static_cast<uint8_t>(v) - 1);
  }

  inline void decode(ArithmeticError &v, scale::Decoder &decoder) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    v = static_cast<ArithmeticError>(decoder.take() + 1);
  }

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, ArithmeticError);

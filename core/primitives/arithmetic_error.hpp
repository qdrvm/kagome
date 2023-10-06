/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_ARITHMETIC_ERROR_HPP
#define KAGOME_CORE_PRIMITIVES_ARITHMETIC_ERROR_HPP

#include <cstdint>

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

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ArithmeticError &v) {
    // index shift is required for compatibility with rust implementation.
    // std::error_code policy preserves 0 index for success cases.
    return s << static_cast<uint8_t>(v) - 1;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ArithmeticError &v) {
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

#endif  // KAGOME_CORE_PRIMITIVES_ARITHMETIC_ERROR_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/arithmetic_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, ArithmeticError, e) {
  using E = kagome::primitives::ArithmeticError;
  switch (e) {
    case E::Underflow:
      return "An underflow would occur";
    case E::Overflow:
      return "An overflow would occur";
    case E::DivisionByZero:
      return "Division by zero";
  }
  return "Unknown ArithmeticError";
}

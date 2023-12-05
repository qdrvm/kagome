/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/erasure_coding_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome, ErasureCodingError, e) {
  auto error_code = kagome::fromErasureCodingError(e);
  return fmt::format("ErasureCodingError({})", static_cast<size_t>(error_code));
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome, ErasureCodingRootError, e) {
  using E = decltype(e);
  switch (e) {
    case E::MISMATCH:
      return "Erasure coding root mismatch";
  }
  abort();
}

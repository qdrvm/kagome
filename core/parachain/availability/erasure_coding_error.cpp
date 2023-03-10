/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/availability/erasure_coding_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome, ErasureCodingError, e) {
  return fmt::format("ErasureCodingError({})", e);
}

OUTCOME_CPP_DEFINE_CATEGORY(kagome, ErasureCodingRootError, e) {
  using E = decltype(e);
  switch (e) {
    case E::MISMATCH:
      return "Erasure coding root mismatch";
  }
  abort();
}

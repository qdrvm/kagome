/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/memory_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, MemoryError, ) {
  return "MemoryError: Memory access out of bounds";
}

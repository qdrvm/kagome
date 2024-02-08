/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/types.hpp"

namespace kagome::runtime {
  inline bool memoryCheck(WasmPointer begin, WasmSize size, WasmSize max) {
    auto end = begin + size;
    return end >= begin and end <= max;
  }
}  // namespace kagome::runtime

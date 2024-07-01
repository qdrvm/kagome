/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/wabt/instrument.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct NoopWasmInstrumenter : WasmInstrumenter {
    WabtOutcome<common::Buffer> instrument(
        common::BufferView code, const MemoryLimits &) const override {
      return common::Buffer{code};
    }
  };
}  // namespace kagome::runtime

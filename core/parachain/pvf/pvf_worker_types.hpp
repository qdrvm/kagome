/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "scale/scale.hpp"
#include "scale/tie.hpp"

namespace kagome::parachain {

  enum class RuntimeEngine : uint8_t {
    kBinaryen = 0,
    kWAVM,
    kWasmEdge,
  };

  struct PvfWorkerInput {
    SCALE_TIE(5);
    RuntimeEngine engine;
    common::Buffer runtime_code;
    std::string function;
    common::Buffer params;
    std::string cache_dir;
  };
}  // namespace kagome::parachain

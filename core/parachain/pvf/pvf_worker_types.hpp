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
    kWasmEdgeInterpreted,
    kWasmEdgeCompiled,
  };

  struct PvfWorkerInput {
    SCALE_TIE(6);

    RuntimeEngine engine;
    common::Buffer runtime_code;
    std::string function;
    common::Buffer params;
    std::optional<std::string> cache_dir;
    std::vector<std::string> log_params;
  };
}  // namespace kagome::parachain

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "common/buffer.hpp"
#include "runtime/runtime_context.hpp"
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
    std::string path_compiled;
    std::string function;
    common::Buffer params;
    std::vector<std::string> log_params;
    bool force_disable_secure_mode;
  };

}  // namespace kagome::parachain

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
#include "scale/std_variant.hpp"
#include "scale/tie.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::parachain {

  enum class RuntimeEngine : uint8_t {
    kBinaryen = 0,
    kWAVM,
    kWasmEdgeInterpreted,
    kWasmEdgeCompiled,
  };

  RuntimeEngine pvf_runtime_engine(
      const application::AppConfiguration &app_config);

  struct PvfWorkerInputConfig {
    SCALE_TIE(4);

    RuntimeEngine engine;
    std::string cache_dir;
    std::vector<std::string> log_params;
    bool force_disable_secure_mode;
  };

  using PvfWorkerInputCode = std::string;

  using PvfWorkerInputArgs = Buffer;

  using PvfWorkerInput = std::variant<PvfWorkerInputCode, PvfWorkerInputArgs>;
}  // namespace kagome::parachain

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "common/buffer.hpp"
#include "parachain/pvf/secure_mode_precheck.hpp"
#include "runtime/runtime_context.hpp"
#include "scale/scale.hpp"

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
    RuntimeEngine engine;
    std::string cache_dir;
    std::vector<std::string> log_params;
    bool force_disable_secure_mode;
    SecureModeSupport secure_mode_support;
  };

  struct PvfWorkerInputCodeParams {
    std::string path;
    kagome::runtime::RuntimeContext::ContextParams context_params;
    bool operator==(const PvfWorkerInputCodeParams &other) const = default;
  };

  using PvfWorkerInputArgs = Buffer;

  using PvfWorkerInput =
      std::variant<PvfWorkerInputCodeParams, PvfWorkerInputArgs>;
}  // namespace kagome::parachain

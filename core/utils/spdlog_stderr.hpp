/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/wasm_compiler_definitions.hpp"  // this header-file is generated

#if KAGOME_WASM_COMPILER_WASM_EDGE
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/spdlog.h>

// fix wasmedge spdlog, change stdout to stderr
inline void spdlogStderr() {
  spdlog::default_logger()->sinks() = {
      std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>()};
}
#else
inline void spdlogStderr() {}
#endif

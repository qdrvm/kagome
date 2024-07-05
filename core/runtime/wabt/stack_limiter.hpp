/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "runtime/wabt/error.hpp"

namespace wabt {
  struct Module;
  struct Func;
}  // namespace wabt

namespace kagome::runtime {
  // for tests
  namespace detail {
    WabtOutcome<uint32_t> compute_stack_cost(const log::Logger &logger,
                                             const wabt::Func &func,
                                             const wabt::Module &module);
  }

  /**
   * Implements the same logic as substrate's
   * https://github.com/paritytech/wasm-instrument Patches the wasm code,
   * wrapping each function call in a check that this call is not going to
   * exceed the global stack limit
   * @param uncompressed_wasm - uncompressed wasm code
   * @param stack_limit - the global stack limit
   * @return patched code or error
   */
  [[nodiscard]] WabtOutcome<common::Buffer> instrumentWithStackLimiter(
      common::BufferView uncompressed_wasm, size_t stack_limit);

  WabtOutcome<void> instrumentWithStackLimiter(wabt::Module &module,
                                               size_t stack_limit);
}  // namespace kagome::runtime

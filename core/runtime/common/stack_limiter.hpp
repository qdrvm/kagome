/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/types.hpp"

namespace wabt {
  struct Module;
  struct Func;
}  // namespace wabt

namespace kagome::runtime {

  struct StackLimiterError {
    [[nodiscard]] const std::string &message() const {
      return msg;
    }
    std::string msg;
  };

  template <typename T>
  using WabtOutcome = outcome::result<T, StackLimiterError>;

  // for tests
  namespace detail {
    outcome::result<uint32_t, StackLimiterError> compute_stack_cost(
        const log::Logger &logger,
        const wabt::Func &func,
        const wabt::Module &module);
  }

  inline boost::exception_ptr make_exception_ptr(const StackLimiterError &e) {
    return std::make_exception_ptr(std::runtime_error{e.msg});
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

  class InstrumentWasm {
   public:
    virtual ~InstrumentWasm() = default;

    virtual WabtOutcome<common::Buffer> instrument(
        common::BufferView code, const MemoryLimits &config) const;
  };
}  // namespace kagome::runtime

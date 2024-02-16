/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::runtime {

  struct StackLimiterError {
    std::string msg;
  };

  inline boost::exception_ptr make_exception_ptr(const StackLimiterError &e) {
    return std::make_exception_ptr(std::runtime_error{e.msg});
  }

  /**
   * Implements the same logic as substrate's https://github.com/paritytech/wasm-instrument
   * Patches the wasm code, wrapping each function call in a check that
   * this call is not going to exceed the global stack limit
   * @param uncompressed_wasm - uncompressed wasm code
   * @param stack_limit - the global stack limit
   * @return patched code or error
   */
  [[nodiscard]] outcome::result<common::Buffer, StackLimiterError>
  instrumentWithStackLimiter(common::BufferView uncompressed_wasm,
                             size_t stack_limit);

}  // namespace kagome::runtime

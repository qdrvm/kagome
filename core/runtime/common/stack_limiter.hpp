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

  [[nodiscard]] outcome::result<common::Buffer, StackLimiterError>
  instrumentWithStackLimiter(common::BufferView uncompressed_wasm,
                             size_t stack_limit);

}  // namespace kagome::runtime
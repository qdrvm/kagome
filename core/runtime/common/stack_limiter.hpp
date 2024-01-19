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

  [[nodiscard]] outcome::result<common::Buffer, StackLimiterError>
  instrumentWithStackLimiter(common::BufferView uncompressed_wasm);

}  // namespace kagome::runtime
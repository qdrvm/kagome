/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/custom.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  struct WabtError {
    [[nodiscard]] const std::string &message() const {
      return msg;
    }
    std::string msg;
  };

  inline auto format_as(const WabtError &e) {
    return e.msg;
  }

  inline std::error_code make_error_code(const WabtError &) {
    return Error::INSTRUMENTATION_FAILED;
  }

  inline void outcome_throw_as_system_error_with_payload(WabtError e) {
    throw e;
  }

  template <typename T>
  using WabtOutcome = CustomOutcome<T, WabtError>;
}  // namespace kagome::runtime

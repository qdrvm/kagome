/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <vector>

#include "runtime/common/runtime_execution_error.hpp"

namespace kagome::runtime {
  template <typename T>
  outcome::result<std::optional<T>> ifExport(outcome::result<T> &&r) {
    if (r) {
      return std::move(r.value());
    }
    if (r.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::nullopt;
    }
    return r.error();
  }

  template <typename T>
  outcome::result<std::vector<T>> ifExportVec(
      outcome::result<std::vector<T>> &&r) {
    if (r) {
      return std::move(r.value());
    }
    if (r.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::vector<T>{};
    }
    return r.error();
  }
}  // namespace kagome::runtime

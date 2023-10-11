/**
* Copyright Quadrivium LLC All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#include "runtime/common/runtime_execution_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, RuntimeExecutionError, e) {
  using E = kagome::runtime::RuntimeExecutionError;
  switch (e) {
    case E::NO_TRANSACTIONS_WERE_STARTED:
      return "No storage transactions were started";
    case E::EXPORT_FUNCTION_NOT_FOUND:
      return "Export function not found";
  }
  return "Unknown RuntimeExecutionError";
}

/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/types.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, Error, e) {
  using E = kagome::runtime::Error;
  switch (e) {
    case E::INSTRUMENTATION_FAILED:
      return "Runtime module instrumentation failed";
    case E::COMPILATION_FAILED:
      return "Runtime module compilation failed";
  }
  return "Unknown module repository error";
}

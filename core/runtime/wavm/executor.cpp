/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/executor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wavm, Executor::Error, e) {
  using E = kagome::runtime::wavm::Executor::Error;
  switch (e) {
    case E::EXECUTION_ERROR:
      return "Error occurred when executing a Runtime method. Check logs for "
             "more details.";
  }
  return "Unknown error";
}

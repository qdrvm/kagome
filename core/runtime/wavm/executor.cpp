/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/executor.hpp"

#include <WAVM/RuntimeABI/RuntimeABI.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime::wavm, Executor::Error, e) {
  using E = kagome::runtime::wavm::Executor::Error;
  switch (e) {
    case E::EXECUTION_ERROR:
      return "Error occurred when executing a Runtime method. Check logs for "
             "more details.";
  }
  return "Unknown error";
}

namespace kagome::runtime::wavm {

  outcome::result<PtrSize> Executor::execute(ModuleInstance &instance, std::string_view name, PtrSize args) {
    try {
      PtrSize result {};
      WAVM::Runtime::unwindSignalsAsExceptions(
          [&result, &instance, &name, &args] {
            result = instance.callExportFunction(name, args);
          });
      return result;
    } catch (WAVM::Runtime::Exception *e) {
      logger_->error(WAVM::Runtime::describeException(e));
      WAVM::Runtime::destroyException(e);
      return Error::EXECUTION_ERROR;
    }
  }

}

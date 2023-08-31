/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_transaction_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, RuntimeTransactionError, e) {
  using E = kagome::runtime::RuntimeTransactionError;
  switch (e) {
    case E::NO_TRANSACTIONS_WERE_STARTED:
      return "no transactions were started";
    case E::EXPORT_FUNCTION_NOT_FOUND:
      return "Export function not found";
  }
  return "unknown TransactionError";
}

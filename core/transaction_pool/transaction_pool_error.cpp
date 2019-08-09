/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/transaction_pool_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::transaction_pool, TransactionPoolError, e) {
  using E = kagome::transaction_pool::TransactionPoolError;
  switch (e) {
    case E::ALREADY_IMPORTED:
      return "Transaction has already been imported to the pool";
  }
  return "Unknown error";
}

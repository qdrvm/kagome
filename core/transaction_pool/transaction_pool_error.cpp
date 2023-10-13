/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/transaction_pool_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::transaction_pool, TransactionPoolError, e) {
  using E = kagome::transaction_pool::TransactionPoolError;
  switch (e) {
    case E::TX_ALREADY_IMPORTED:
      return "Transaction has already been imported to the pool";
    case E::TX_NOT_FOUND:
      return "Transaction not found in the pool";
    case E::POOL_IS_FULL:
      return "Transaction pool is full";
  }
  return "Unknown transaction pool error";
}

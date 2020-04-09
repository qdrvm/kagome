/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

#include "transaction_pool/transaction_pool.hpp"

#include <gmock/gmock.h>

namespace kagome::transaction_pool {

  class TransactionPoolMock : public TransactionPool {
   public:
    MOCK_METHOD1(submitOne, outcome::result<void>(Transaction));
    MOCK_METHOD1(submit, outcome::result<void>(std::vector<Transaction>));
    MOCK_METHOD0(getReadyTransactions, std::vector<Transaction>());

    MOCK_METHOD1(
        removeStale,
        outcome::result<std::vector<Transaction>>(const primitives::BlockId &));

    MOCK_METHOD3(
        pruneTag,
        std::vector<Transaction>(const primitives::BlockId &at,
                                 const Transaction::Tag &,
                                 const std::vector<common::Hash256> &));
    MOCK_METHOD2(pruneTag,
                 std::vector<Transaction>(const primitives::BlockId &at,
                                          const Transaction::Tag &));

    MOCK_CONST_METHOD0(getStatus, Status());
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

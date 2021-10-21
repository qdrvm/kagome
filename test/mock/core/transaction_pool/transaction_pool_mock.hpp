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
    MOCK_CONST_METHOD0(getPendingTransactions,
                       std::unordered_map<Transaction::Hash,
                                          std::shared_ptr<Transaction>> &());

    outcome::result<void> submitOne(Transaction &&tx) {
      return submitOne(tx);
    }
    MOCK_METHOD1(submitOne, outcome::result<void>(Transaction));
    MOCK_METHOD1(submit, outcome::result<void>(std::vector<Transaction>));

    MOCK_METHOD1(removeOne,
                 outcome::result<Transaction>(const Transaction::Hash &));
    MOCK_METHOD1(remove, void(const std::vector<Transaction::Hash> &));

    MOCK_CONST_METHOD0(
        getReadyTransactions,
        std::map<Transaction::Hash, std::shared_ptr<Transaction>>());

    MOCK_METHOD1(
        removeStale,
        outcome::result<std::vector<Transaction>>(const primitives::BlockId &));

    MOCK_CONST_METHOD0(getStatus, Status());
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

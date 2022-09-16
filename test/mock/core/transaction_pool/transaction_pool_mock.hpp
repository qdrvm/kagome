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
    MOCK_METHOD(
        (std::unordered_map<Transaction::Hash, std::shared_ptr<Transaction>> &),
        getPendingTransactions,
        (),
        (const));

    MOCK_METHOD(outcome::result<Transaction::Hash>,
                submitExtrinsic,
                (primitives::TransactionSource, primitives::Extrinsic),
                (override));

    MOCK_METHOD(outcome::result<void>, submitOne, (Transaction), ());
    outcome::result<void> submitOne(Transaction &&tx) override {
      return submitOne(tx);
    }

    MOCK_METHOD(outcome::result<void>, submit, (std::vector<Transaction>), ());

    MOCK_METHOD(outcome::result<Transaction>,
                removeOne,
                (const Transaction::Hash &),
                (override));

    MOCK_METHOD(void, remove, (const std::vector<Transaction::Hash> &), ());

    MOCK_METHOD((std::map<Transaction::Hash, std::shared_ptr<Transaction>>),
                getReadyTransactions,
                (),
                (const));

    MOCK_METHOD(outcome::result<std::vector<Transaction>>,
                removeStale,
                (const primitives::BlockId &),
                (override));

    MOCK_METHOD(Status, getStatus, (), (const, override));

    MOCK_METHOD(outcome::result<primitives::Transaction>,
                constructTransaction,
                (primitives::TransactionSource, primitives::Extrinsic),
                (const, override));
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

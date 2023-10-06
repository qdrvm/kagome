/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "transaction_pool/transaction_pool.hpp"

#include <gmock/gmock.h>

namespace kagome::transaction_pool {

  class TransactionPoolMock : public TransactionPool {
   public:
    MOCK_METHOD(void,
                getPendingTransactions,
                (TransactionPool::TxRequestCallback &&),
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

    MOCK_METHOD(void,
                getReadyTransactions,
                (TransactionPool::TxRequestCallback &&),
                (const));

    MOCK_METHOD((std::vector<std::pair<Transaction::Hash,
                                       std::shared_ptr<const Transaction>>>),
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

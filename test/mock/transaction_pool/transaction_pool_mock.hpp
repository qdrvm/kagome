/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP
#define KAGOME_TEST_MOCK_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

#include <gmock/gmock.h>
#include "primitives/transaction.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::transaction_pool {
  class TransactionPoolMock : public TransactionPool {
   protected:
    using Transaction = primitives::Transaction;
    using Extrinsic = primitives::Extrinsic;
    using TransactionTag = primitives::TransactionTag;
    using BlockId = primitives::BlockId;

   public:
    ~TransactionPoolMock() override = default;

    // only this one we need here
    MOCK_METHOD1(submitOne, outcome::result<void>(Transaction));
    MOCK_METHOD1(submit, outcome::result<void>(std::vector<Transaction>));
    MOCK_METHOD0(getReadyTransactions, std::vector<Transaction>());
    MOCK_METHOD1(removeStale, std::vector<Transaction>(const BlockId &));
    MOCK_METHOD3(pruneTag,
                 std::vector<Transaction>(const primitives::BlockId &at, const primitives::TransactionTag &tag,
                     const std::vector<common::Hash256> &known_imported_hashes));
    MOCK_CONST_METHOD0(getStatus, Status(void));
  };
}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

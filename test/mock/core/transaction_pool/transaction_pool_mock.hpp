/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

#include "transaction_pool/transaction_pool.hpp"

#include <gmock/gmock.h>

namespace kagome::transaction_pool {

  class TransactionPoolMock : public TransactionPool {
   public:
    MOCK_METHOD1(submitOne, outcome::result<void>(primitives::Transaction));
    MOCK_METHOD1(submit,
                 outcome::result<void>(std::vector<primitives::Transaction>));
    MOCK_METHOD0(getReadyTransactions, std::vector<primitives::Transaction>());

    MOCK_METHOD1(removeStale,
                 outcome::result<std::vector<primitives::Transaction>>(
                     const primitives::BlockId &));

    MOCK_METHOD3(pruneTag,
                 std::vector<primitives::Transaction>(
                     const primitives::BlockId &at,
                     const primitives::TransactionTag &,
                     const std::vector<common::Hash256> &));
    MOCK_METHOD2(pruneTag,
                 std::vector<primitives::Transaction>(
                     const primitives::BlockId &at,
                     const primitives::TransactionTag &));

    MOCK_CONST_METHOD0(getStatus, Status());
  };

}  // namespace kagome::transaction_pool

#endif  // KAGOME_TEST_MOCK_CORE_TRANSACTION_POOL_TRANSACTION_POOL_MOCK_HPP

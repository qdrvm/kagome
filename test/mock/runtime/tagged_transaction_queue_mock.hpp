/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_RUNTIME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP
#define KAGOME_TEST_MOCK_RUNTIME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

#include <gmock/gmock.h>
#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::runtime {
  class TaggedTransactionQueueMock : public TaggedTransactionQueue {
    using TransactionValidity = primitives::TransactionValidity;
    using Extrinsic = primitives::Extrinsic;

   public:
    ~TaggedTransactionQueueMock() override = default;

    MOCK_METHOD1(validate_transaction,
                 outcome::result<TransactionValidity>(const Extrinsic &));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_RUNTIME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

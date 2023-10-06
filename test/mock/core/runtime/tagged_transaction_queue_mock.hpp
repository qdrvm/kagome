/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

#include "runtime/runtime_api/tagged_transaction_queue.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct TaggedTransactionQueueMock : public TaggedTransactionQueue {
    MOCK_METHOD(outcome::result<TransactionValidityAt>,
                validate_transaction,
                (primitives::TransactionSource, const primitives::Extrinsic &),
                (override));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

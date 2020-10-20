/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

#include "runtime/tagged_transaction_queue.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct TaggedTransactionQueueMock : public TaggedTransactionQueue {
    MOCK_METHOD2(validate_transaction,
                 outcome::result<primitives::TransactionValidity>(
                     primitives::TransactionSource,
                     const primitives::Extrinsic &));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

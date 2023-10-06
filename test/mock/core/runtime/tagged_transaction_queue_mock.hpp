/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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

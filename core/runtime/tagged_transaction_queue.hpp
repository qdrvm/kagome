/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

#include "primitives/extrinsic.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::runtime {

  class TaggedTransactionQueue {
   public:
    virtual ~TaggedTransactionQueue() = default;

    virtual outcome::result<primitives::TransactionValidity>
    validate_transaction(const primitives::Extrinsic &ext) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::runtime {

  /**
   * Part of runtime API responsible for transaction validation
   */
  class TaggedTransactionQueue {
   public:
    virtual ~TaggedTransactionQueue() = default;

    /**
     * Calls the TaggedTransactionQueue_validate_transaction function from wasm
     * code
     * @param ext extrinsic containing transaction to be validated
     * @return structure with information about transaction validity
     */
    virtual outcome::result<primitives::TransactionValidity>
    validate_transaction(primitives::TransactionSource source,
                        const primitives::Extrinsic &ext) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

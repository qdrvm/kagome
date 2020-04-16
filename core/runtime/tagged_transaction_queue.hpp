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
    validate_transaction(const primitives::Extrinsic &ext) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_HPP

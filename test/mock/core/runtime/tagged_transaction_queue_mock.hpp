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

#ifndef KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP
#define KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

#include "runtime/tagged_transaction_queue.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct TaggedTransactionQueueMock : public TaggedTransactionQueue {
    MOCK_METHOD1(validate_transaction,
                 outcome::result<primitives::TransactionValidity>(
                     const primitives::Extrinsic &));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TAGGED_TRANSACTION_QUEUE_MOCK_HPP

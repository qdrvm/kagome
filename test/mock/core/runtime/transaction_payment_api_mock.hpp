/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_TRANSACTION_PAYMENT_API_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_TRANSACTION_PAYMENT_API_MOCK_HPP

#include "runtime/runtime_api/transaction_payment_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class TransactionPaymentApiMock : public TransactionPaymentApi {
   public:
    MOCK_METHOD(
        outcome::result<primitives::RuntimeDispatchInfo<primitives::OldWeight>>,
        query_info,
        (const primitives::BlockHash &block,
         const primitives::Extrinsic &ext,
         uint32_t len),
        (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_TRANSACTION_PAYMENT_API_MOCK_HPP

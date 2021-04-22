/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_TRANSACTION_PAYMENT_API_HPP
#define KAGOME_RUNTIME_WAVM_TRANSACTION_PAYMENT_API_HPP

#include "runtime/transaction_payment_api.hpp"
#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  class WavmTransactionPaymentApi final: public TransactionPaymentApi {
   public:
    WavmTransactionPaymentApi(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)} {
      BOOST_ASSERT(executor_);
    }

    outcome::result<primitives::RuntimeDispatchInfo> query_info(
        const primitives::Extrinsic &ext, uint32_t len) override {
      return executor_->call<primitives::RuntimeDispatchInfo>(
          "TransactionPaymentApi_query_info", ext, len);
    }

   private:
    std::shared_ptr<Executor> executor_;

  };

}  // namespace kagome::runtime

#endif  // KAGOME_TRANSACTION_PAYMENT_API_HPP

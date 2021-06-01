/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_WAVM_TRANSACTION_PAYMENT_API_HPP
#define KAGOME_RUNTIME_WAVM_TRANSACTION_PAYMENT_API_HPP

#include "runtime/transaction_payment_api.hpp"

namespace kagome::runtime::wavm {

  class Executor;

  class WavmTransactionPaymentApi final : public TransactionPaymentApi {
   public:
    WavmTransactionPaymentApi(std::shared_ptr<Executor> executor);

    outcome::result<primitives::RuntimeDispatchInfo> query_info(
        const primitives::Extrinsic &ext, uint32_t len) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_TRANSACTION_PAYMENT_API_HPP

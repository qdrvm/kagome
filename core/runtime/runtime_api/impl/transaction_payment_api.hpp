/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_TRANSACTION_PAYMENT_API_HPP
#define KAGOME_RUNTIME_IMPL_TRANSACTION_PAYMENT_API_HPP

#include "runtime/runtime_api/transaction_payment_api.hpp"

namespace kagome::runtime {

  class Executor;

  class TransactionPaymentApiImpl final : public TransactionPaymentApi {
   public:
    explicit TransactionPaymentApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<primitives::RuntimeDispatchInfo> query_info(
        const primitives::BlockHash &block,
        const primitives::Extrinsic &ext,
        uint32_t len) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TRANSACTION_PAYMENT_API_HPP

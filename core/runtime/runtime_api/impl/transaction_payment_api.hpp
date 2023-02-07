/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_IMPL_TRANSACTION_PAYMENT_API_HPP
#define KAGOME_RUNTIME_IMPL_TRANSACTION_PAYMENT_API_HPP

#include "primitives/version.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_api/transaction_payment_api.hpp"

namespace kagome::runtime {

  class Executor;

  class TransactionPaymentApiImpl final : public TransactionPaymentApi {
   public:
    enum class Error {
      // Transaction payment runtime API is not found in the runtime
      TRANSACTION_PAYMENT_API_NOT_FOUND = 1,
    };

    explicit TransactionPaymentApiImpl(std::shared_ptr<Executor> executor,
                                       std::shared_ptr<Core> core_api,
                                       std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<primitives::RuntimeDispatchInfo<primitives::OldWeight>>
    query_info(const primitives::BlockHash &block,
               const primitives::Extrinsic &ext,
               uint32_t len) override;

   private:
    std::shared_ptr<Executor> executor_;
    std::shared_ptr<Core> core_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, TransactionPaymentApiImpl::Error);

#endif  // KAGOME_TRANSACTION_PAYMENT_API_HPP

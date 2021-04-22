/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_PAYMENT_API_IMPL_HPP
#define KAGOME_TRANSACTION_PAYMENT_API_IMPL_HPP

#include "runtime/binaryen/runtime_api/runtime_api.hpp"
#include "runtime/transaction_payment_api.hpp"
#include "scale/types.hpp"

namespace kagome::runtime {
  class RuntimeCodeProvider;
}  // namespace kagome::runtime

namespace kagome::runtime::binaryen {

  class TransactionPaymentApiImpl : public RuntimeApi,
                                    public TransactionPaymentApi {
   public:
    TransactionPaymentApiImpl(
        const std::shared_ptr<RuntimeEnvironmentFactory> &runtime_env_factory);

    ~TransactionPaymentApiImpl() override = default;

    outcome::result<primitives::RuntimeDispatchInfo> query_info(
        const primitives::Extrinsic &ext, uint32_t len) override;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_TRANSACTION_PAYMENT_API_IMPL_HPP

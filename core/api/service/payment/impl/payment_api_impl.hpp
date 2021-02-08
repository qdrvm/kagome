/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PAYMENT_API_IMPL_HPP
#define KAGOME_PAYMENT_API_IMPL_HPP

#include "api/service/payment/payment_api.hpp"

namespace kagome::runtime {
  class TransactionPaymentApi;
}

namespace kagome::api {

  class PaymentApiImpl : public PaymentApi {
   public:
    PaymentApiImpl(std::shared_ptr<runtime::TransactionPaymentApi> api);
    ~PaymentApiImpl() override = default;

    outcome::result<primitives::RuntimeDispatchInfo> queryInfo(
        const primitives::Extrinsic &extrinsic,
        uint32_t len,
        OptionalHashRef at)
        const override;

   private:
    std::shared_ptr<runtime::TransactionPaymentApi> api_;
  };

}  // namespace kagome::api

#endif  // KAGOME_PAYMENT_API_IMPL_HPP

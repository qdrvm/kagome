/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/payment/impl/payment_api_impl.hpp"

#include "runtime/transaction_payment_api.hpp"

namespace kagome::api {

  PaymentApiImpl::PaymentApiImpl(
      std::shared_ptr<runtime::TransactionPaymentApi> api)
      : api_(std::move(api)) {
    BOOST_ASSERT(api_);
  }

  primitives::RuntimeDispatchInfo PaymentApiImpl::queryInfo(
      const primitives::Extrinsic &extrinsic, OptionalHashRef at) const {
    auto res = api_->query_info(extrinsic);
    return {.weight = 0xBEEF,
            .dispatch_class =
                primitives::RuntimeDispatchInfo::DispatchClass::Normal,
            .partial_fee = 0xDEAD};
  }

}  // namespace kagome::api
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/payment/impl/payment_api_impl.hpp"

#include "runtime/runtime_api/transaction_payment_api.hpp"
#include "scale/types.hpp"

namespace kagome::api {

  PaymentApiImpl::PaymentApiImpl(
      std::shared_ptr<runtime::TransactionPaymentApi> api)
      : api_(std::move(api)) {
    BOOST_ASSERT(api_);
  }

  outcome::result<primitives::RuntimeDispatchInfo> PaymentApiImpl::queryInfo(
      const primitives::Extrinsic &extrinsic, uint32_t len, OptionalHashRef at) const {
    return api_->query_info(extrinsic, len);
  }

}  // namespace kagome::api

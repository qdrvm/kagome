/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/transaction_payment_api.hpp"

#include "runtime/common/executor.hpp"

namespace kagome::runtime {

  TransactionPaymentApiImpl::TransactionPaymentApiImpl(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::RuntimeDispatchInfo>
  TransactionPaymentApiImpl::query_info(const primitives::BlockHash &block,
                                        const primitives::Extrinsic &ext,
                                        uint32_t len) {
    return executor_->callAt<primitives::RuntimeDispatchInfo>(
        block, "TransactionPaymentApi_query_info", ext, len);
  }

}  // namespace kagome::runtime

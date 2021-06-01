/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/runtime_api/transaction_payment_api.hpp"

#include "runtime/wavm/executor.hpp"

namespace kagome::runtime::wavm {

  WavmTransactionPaymentApi::WavmTransactionPaymentApi(
      std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<primitives::RuntimeDispatchInfo>
  WavmTransactionPaymentApi::query_info(const primitives::Extrinsic &ext,
                                        uint32_t len) {
    return executor_->callAtLatest<primitives::RuntimeDispatchInfo>(
        "TransactionPaymentApi_query_info", ext, len);
  }

}  // namespace kagome::runtime::wavm

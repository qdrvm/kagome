/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/runtime_api/transaction_payment_api_impl.hpp"

namespace kagome::runtime::binaryen {

outcome::result<primitives::RuntimeDispatchInfo> TransactionPaymentApiImpl::query_info(
        const primitives::Extrinsic &ext, uint32_t len) {
      return execute<primitives::RuntimeDispatchInfo>(
          "TransactionPaymentApi_query_info",
          CallPersistency::EPHEMERAL,
          gsl::span<const uint8_t>(ext.data.data(), ext.data.size()),
          len);
    }
}

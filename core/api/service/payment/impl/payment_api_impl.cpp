/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/payment/impl/payment_api_impl.hpp"

#include "blockchain/block_tree.hpp"
#include "runtime/runtime_api/transaction_payment_api.hpp"
#include "scale/types.hpp"

namespace kagome::api {

  PaymentApiImpl::PaymentApiImpl(
      std::shared_ptr<runtime::TransactionPaymentApi> api,
      std::shared_ptr<const blockchain::BlockTree> block_tree)
      : api_(std::move(api)), block_tree_{std::move(block_tree)} {
    BOOST_ASSERT(api_);
    BOOST_ASSERT(block_tree_);
  }

  outcome::result<primitives::RuntimeDispatchInfo> PaymentApiImpl::queryInfo(
      const primitives::Extrinsic &extrinsic,
      uint32_t len,
      OptionalHashRef at) const {
    if (at.has_value()) {
      return api_->query_info(at.value(), extrinsic, len);
    }
    return api_->query_info(block_tree_->bestLeaf().hash, extrinsic, len);
  }

}  // namespace kagome::api

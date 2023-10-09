/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/extrinsic_observer_impl.hpp"

#include "primitives/transaction_validity.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::network {

  ExtrinsicObserverImpl::ExtrinsicObserverImpl(
      std::shared_ptr<kagome::transaction_pool::TransactionPool> pool)
      : pool_(std::move(pool)) {
    BOOST_ASSERT(pool_);
  }

  outcome::result<common::Hash256> ExtrinsicObserverImpl::onTxMessage(
      const primitives::Extrinsic &extrinsic) {
    return pool_->submitExtrinsic(primitives::TransactionSource::External,
                                  extrinsic);
  }

}  // namespace kagome::network

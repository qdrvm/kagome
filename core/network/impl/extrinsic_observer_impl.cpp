/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/extrinsic_observer_impl.hpp"

#include "api/service/author/author_api.hpp"

namespace kagome::network {

  ExtrinsicObserverImpl::ExtrinsicObserverImpl(
      std::shared_ptr<api::AuthorApi> api)
      : api_(std::move(api)) {
    BOOST_ASSERT(api_);
  }

  outcome::result<common::Hash256> ExtrinsicObserverImpl::onTxMessage(
      const primitives::Extrinsic &extrinsic) {
    return api_->submitExtrinsic(primitives::TransactionSource::External,
                                 extrinsic);
  }

}  // namespace kagome::network

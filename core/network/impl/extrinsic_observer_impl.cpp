/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/extrinsic_observer_impl.hpp"

kagome::network::ExtrinsicObserverImpl::ExtrinsicObserverImpl(
    std::shared_ptr<api::AuthorApi> api)
    : api_(std::move(api)) {
  BOOST_ASSERT(api_);
}

outcome::result<kagome::common::Hash256>
kagome::network::ExtrinsicObserverImpl::onTxMessage(
    const kagome::primitives::Extrinsic &extrinsic) {
  return api_->submitExtrinsic(extrinsic);
}

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "author_api_observer_impl.hpp"

kagome::api::AuthorApiObserverImpl::AuthorApiObserverImpl(
    std::shared_ptr<AuthorApi> api)
    : api_(std::move(api)) {
  BOOST_ASSERT(api_);
}

outcome::result<kagome::common::Hash256>
kagome::api::AuthorApiObserverImpl::onTxMessage(
    const kagome::primitives::Extrinsic &extrinsic) {
  return api_->submitExtrinsic(extrinsic);
}

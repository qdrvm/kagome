/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVISE_AUTHOR_IMPL_AUTHOR_API_OBSERVER_IMPL_HPP
#define KAGOME_CORE_API_SERVISE_AUTHOR_IMPL_AUTHOR_API_OBSERVER_IMPL_HPP

#include "api/service/author/author_api.hpp"
#include "api/service/author/author_api_observer.hpp"
#include "common/logger.hpp"

namespace kagome::api {

  class AuthorApiObserverImpl : public AuthorApiObserver {
   public:
    AuthorApiObserverImpl(std::shared_ptr<AuthorApi> api);
    ~AuthorApiObserverImpl() = default;

    outcome::result<common::Hash256> onTxMessage(
        const primitives::Extrinsic &extrinsic);

   private:
    std::shared_ptr<AuthorApi> api_;
    common::Logger logger_;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVISE_AUTHOR_IMPL_AUTHOR_API_OBSERVER_IMPL_HPP

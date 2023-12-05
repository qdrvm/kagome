/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api::author::request {

  class UnwatchExtrinsic final
      : public details::RequestType<bool, primitives::SubscriptionId> {
   public:
    explicit UnwatchExtrinsic(std::shared_ptr<AuthorApi> &api) : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<Return> execute() override {
      return api_->unwatchExtrinsic(getParam<0>());
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

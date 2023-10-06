/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ACCOUNT_NEXT_INDEX_HPP
#define KAGOME_ACCOUNT_NEXT_INDEX_HPP

#include "api/service/base_request.hpp"
#include "api/service/system/system_api.hpp"

namespace kagome::api::system::request {

  class AccountNextIndex final
      : public details::RequestType<primitives::AccountNonce, std::string> {
   public:
    explicit AccountNextIndex(std::shared_ptr<SystemApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<primitives::AccountNonce> execute() override {
      return api_->getNonceFor(getParam<0>());
    }

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request

#endif  // KAGOME_ACCOUNT_NEXT_INDEX_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "api/service/base_request.hpp"
#include "crypto/key_store/key_type.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::author::request {

  class HasKey final
      : public details::RequestType<bool, std::string, std::string> {
   public:
    explicit HasKey(std::shared_ptr<AuthorApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<Return> execute() override {
      OUTCOME_TRY(public_key, common::unhexWith0x(getParam<0>()));
      if (auto key_type = crypto::KeyType::fromString(getParam<1>())) {
        return api_->hasKey(public_key, *key_type);
      } else {
        return crypto::KeyTypeError::UNSUPPORTED_KEY_TYPE;
      }
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

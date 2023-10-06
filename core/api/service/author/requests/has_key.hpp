/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "api/service/base_request.hpp"
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
      return api_->hasKey(gsl::span(public_key.data(), public_key.size()),
                          crypto::decodeKeyTypeIdFromStr(getParam<1>()));
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

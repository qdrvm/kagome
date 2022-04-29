/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQUESTS_INSERT_KEY_HPP
#define KAGOME_REQUESTS_INSERT_KEY_HPP

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "api/service/base_request.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::author::request {

  class InsertKey final
      : public details::
            RequestType<void, std::string, std::string, std::string> {
   public:
    explicit InsertKey(std::shared_ptr<AuthorApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<Return> execute() override {
      OUTCOME_TRY(seed, common::unhexWith0x(getParam<1>()));
      OUTCOME_TRY(public_key, common::unhexWith0x(getParam<2>()));
      return api_->insertKey(crypto::decodeKeyTypeIdFromStr(getParam<0>()),
                             gsl::span(seed.data(), seed.size()),
                             gsl::span(public_key.data(), public_key.size()));
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

#endif  // KAGOME_REQUESTS_INSERT_KEY_HPP

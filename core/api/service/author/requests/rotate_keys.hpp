/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_REQUESTS_ROTATE_KEYS_HPP
#define KAGOME_REQUESTS_ROTATE_KEYS_HPP

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "api/service/base_request.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::author::request {

  class RotateKeys final : public details::RequestType<common::Buffer> {
   public:
    explicit RotateKeys(std::shared_ptr<AuthorApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<Return> execute() override {
      return api_->rotateKeys();
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

#endif  // KAGOME_REQUESTS_ROTATE_KEYS_HPP

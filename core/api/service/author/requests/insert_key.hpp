/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "api/service/base_request.hpp"
#include "crypto/common.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::author::request {

  class InsertKey final
      : public details::RequestType<
            void,
            std::string,
            std::vector<char, crypto::SecureHeapAllocator<char>>,
            std::string> {
   public:
    explicit InsertKey(std::shared_ptr<AuthorApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<Return> execute() override {
      auto &seed_hex = getParam<1>();
      crypto::SecureBuffer<> seed_buf(seed_hex.size() / 2 - 1);
      OUTCOME_TRY(common::unhexWith0x(
          std::string_view{seed_hex.data(), seed_hex.size()},
          seed_buf.begin()));
      OUTCOME_TRY(public_key, common::unhexWith0x(getParam<2>()));
      return api_->insertKey(crypto::decodeKeyTypeFromStr(getParam<0>()),
                             std::move(seed_buf),
                             public_key);
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

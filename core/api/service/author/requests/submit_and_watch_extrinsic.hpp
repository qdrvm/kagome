/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SUBMIT_AND_WATCH_EXTRINSIC_HPP
#define KAGOME_SUBMIT_AND_WATCH_EXTRINSIC_HPP

#include <jsonrpc-lean/request.h>

#include "api/service/author/author_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api::author::request {

  class SubmitAndWatchExtrinsic final
      : public details::RequestType<primitives::SubscriptionId, std::string> {
   public:
    explicit SubmitAndWatchExtrinsic(std::shared_ptr<AuthorApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };

    outcome::result<primitives::SubscriptionId> execute() override {
      auto ext_hex = getParam<0>();
      OUTCOME_TRY(buffer, common::unhexWith0x(ext_hex));
      OUTCOME_TRY(extrinsic, scale::decode<primitives::Extrinsic>(buffer));
      return api_->submitAndWatchExtrinsic(extrinsic);
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request

#endif  // KAGOME_SUBMIT_AND_WATCH_EXTRINSIC_HPP

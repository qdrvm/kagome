/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/base_request.hpp"

#include "api/service/chain/chain_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api::chain::request {

  struct SubscribeNewHeads final : details::RequestType<uint32_t> {
    explicit SubscribeNewHeads(std::shared_ptr<ChainApi> &api) : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<Return> execute() override {
      return api_->subscribeNewHeads();
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

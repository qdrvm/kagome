/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_SUBSCRIBE_NEW_HEADS_HPP
#define KAGOME_CHAIN_SUBSCRIBE_NEW_HEADS_HPP

#include "api/service/chain/requests/base_request.hpp"

namespace kagome::api::chain::request {

  struct SubscribeNewHeads final : RequestType<uint32_t> {
    using ReturnType = uint32_t;
    using BaseType = RequestType<ReturnType>;

    explicit SubscribeNewHeads(std::shared_ptr<ChainApi> &api) : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<ReturnType> execute() override {
      return api_->subscribeNewHeads();
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_SUBSCRIBE_NEW_HEADS_HPP

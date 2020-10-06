/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_UNSUBSCRIBE_NEW_HEADS_HPP
#define KAGOME_CHAIN_UNSUBSCRIBE_NEW_HEADS_HPP

#include "api/service/base_request.hpp"

namespace kagome::api::chain::request {

  struct UnsubscribeNewHeads final : RequestType<bool, int64_t> {
    using ReturnType = bool;
    using BaseType = RequestType<ReturnType>;

    explicit UnsubscribeNewHeads(std::shared_ptr<ChainApi> &api) : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<ReturnType> execute() override {
      return api_->unsubscribeNewHeads(getParam<0>());
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_UNSUBSCRIBE_NEW_HEADS_HPP

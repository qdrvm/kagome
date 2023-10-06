/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_GET_BLOCK_HPP
#define KAGOME_CHAIN_GET_BLOCK_HPP

#include "api/service/base_request.hpp"

namespace kagome::api::chain::request {

  struct GetBlock final : details::RequestType<primitives::BlockData,
                                               std::optional<std::string>> {
    explicit GetBlock(std::shared_ptr<ChainApi> &api) : api_(api) {}

    outcome::result<primitives::BlockData> execute() override {
      if (const auto &param_0 = getParam<0>()) {
        return api_->getBlock(*param_0);
      }
      return api_->getBlock();
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_GET_BLOCK_HPP

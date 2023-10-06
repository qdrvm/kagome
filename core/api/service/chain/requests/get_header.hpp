/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_GET_HEADER_HPP
#define KAGOME_CHAIN_GET_HEADER_HPP

#include "api/service/base_request.hpp"

namespace kagome::api::chain::request {

  struct GetHeader final : details::RequestType<primitives::BlockHeader,
                                                std::optional<std::string>> {
    explicit GetHeader(std::shared_ptr<ChainApi> &api) : api_(api) {}

    outcome::result<primitives::BlockHeader> execute() override {
      if (const auto &param_0 = getParam<0>()) {
        return api_->getHeader(*param_0);
      }
      return api_->getHeader();
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_GET_HEADER_HPP

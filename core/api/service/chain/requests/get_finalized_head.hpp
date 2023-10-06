/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SERVICE_CHAIN_GET_FINALIZED_HEAD_HPP
#define KAGOME_API_SERVICE_CHAIN_GET_FINALIZED_HEAD_HPP

#include "api/service/base_request.hpp"

namespace kagome::api::chain::request {

  struct GetFinalizedHead final : details::RequestType<primitives::BlockHash> {
    explicit GetFinalizedHead(std::shared_ptr<ChainApi> &api) : api_(api) {}

    outcome::result<primitives::BlockHash> execute() override {
      return api_->getFinalizedHead();
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_API_SERVICE_CHAIN_GET_FINALIZED_HEAD_HPP

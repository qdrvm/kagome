/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_UNSUBSCRIBE_FINALIZED_HEADS_HPP
#define KAGOME_CHAIN_UNSUBSCRIBE_FINALIZED_HEADS_HPP

#include "api/service/base_request.hpp"

namespace kagome::api::chain::request {

  struct UnsubscribeFinalizedHeads final
      : details::RequestType<void, uint32_t> {
    explicit UnsubscribeFinalizedHeads(std::shared_ptr<ChainApi> &api)
        : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<Return> execute() override {
      return api_->unsubscribeFinalizedHeads(getParam<0>());
    }

   private:
    std::shared_ptr<ChainApi> api_;
  };

}  // namespace kagome::api::chain::request

#endif  // KAGOME_CHAIN_UNSUBSCRIBE_FINALIZED_HEADS_HPP

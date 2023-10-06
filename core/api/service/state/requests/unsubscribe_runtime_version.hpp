/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_REQUEST_UNSUBSCRIBERUNTIMEVERSION
#define KAGOME_API_STATE_REQUEST_UNSUBSCRIBERUNTIMEVERSION

#include "api/service/base_request.hpp"

#include "api/service/state/state_api.hpp"

namespace kagome::api::state::request {

  struct UnsubscribeRuntimeVersion final
      : details::RequestType<void, uint32_t> {
    explicit UnsubscribeRuntimeVersion(std::shared_ptr<StateApi> &api)
        : api_(api) {
      BOOST_ASSERT(api_);
    }

    outcome::result<Return> execute() override {
      return api_->unsubscribeRuntimeVersion(getParam<0>());
    }

   private:
    std::shared_ptr<StateApi> api_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_STATE_REQUEST_UNSUBSCRIBERUNTIMEVERSION

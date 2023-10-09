/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include <optional>
#include <vector>

#include "api/service/base_request.hpp"
#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  struct GetMetadata final
      : details::RequestType<std::string, std::optional<std::string>> {
   public:
    explicit GetMetadata(std::shared_ptr<StateApi> api)
        : api_(std::move(api)){};

    outcome::result<Return> execute() {
      if (const auto &param_0 = getParam<0>()) {
        return api_->getMetadata(*param_0);
      }
      return api_->getMetadata();
    }

   private:
    std::shared_ptr<StateApi> api_;
  };

}  // namespace kagome::api::state::request

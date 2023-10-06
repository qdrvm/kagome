/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class GetRuntimeVersion final {
   public:
    GetRuntimeVersion(const GetRuntimeVersion &) = delete;
    GetRuntimeVersion &operator=(const GetRuntimeVersion &) = delete;

    GetRuntimeVersion(GetRuntimeVersion &&) = default;
    GetRuntimeVersion &operator=(GetRuntimeVersion &&) = default;

    explicit GetRuntimeVersion(std::shared_ptr<StateApi> api);
    ~GetRuntimeVersion() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);
    outcome::result<primitives::Version> execute();

   private:
    std::shared_ptr<StateApi> api_;
    std::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

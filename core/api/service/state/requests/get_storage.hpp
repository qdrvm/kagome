/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include <optional>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class GetStorage final {
   public:
    GetStorage(const GetStorage &) = delete;
    GetStorage &operator=(const GetStorage &) = delete;

    GetStorage(GetStorage &&) = default;
    GetStorage &operator=(GetStorage &&) = default;

    explicit GetStorage(std::shared_ptr<StateApi> api) : api_(std::move(api)){};
    ~GetStorage() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<common::Buffer>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <jsonrpc-lean/request.h>

#include "api/service/state/state_api.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::state::request {

  class GetStorageSize final {
   public:
    GetStorageSize(const GetStorageSize &) = delete;
    GetStorageSize &operator=(const GetStorageSize &) = delete;

    GetStorageSize(GetStorageSize &&) = default;
    GetStorageSize &operator=(GetStorageSize &&) = default;

    explicit GetStorageSize(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) {};
    ~GetStorageSize() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<uint64_t>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

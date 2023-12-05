/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <jsonrpc-lean/request.h>

#include "api/service/child_state/child_state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::child_state::request {

  class GetStorageSize final {
   public:
    GetStorageSize(const GetStorageSize &) = delete;
    GetStorageSize &operator=(const GetStorageSize &) = delete;

    GetStorageSize(GetStorageSize &&) = default;
    GetStorageSize &operator=(GetStorageSize &&) = default;

    explicit GetStorageSize(std::shared_ptr<ChildStateApi> api)
        : api_(std::move(api)){};
    ~GetStorageSize() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<uint64_t>> execute();

   private:
    std::shared_ptr<ChildStateApi> api_;
    common::Buffer child_storage_key_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::child_state::request

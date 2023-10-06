/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_HASH
#define KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_HASH

#include <optional>

#include <jsonrpc-lean/request.h>

#include "api/service/child_state/child_state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::child_state::request {

  class GetStorageHash final {
   public:
    GetStorageHash(const GetStorageHash &) = delete;
    GetStorageHash &operator=(const GetStorageHash &) = delete;

    GetStorageHash(GetStorageHash &&) = default;
    GetStorageHash &operator=(GetStorageHash &&) = default;

    explicit GetStorageHash(std::shared_ptr<ChildStateApi> api)
        : api_(std::move(api)){};
    ~GetStorageHash() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<kagome::primitives::BlockHash>> execute();

   private:
    std::shared_ptr<ChildStateApi> api_;
    common::Buffer child_storage_key_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::child_state::request

#endif  // KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_HASH

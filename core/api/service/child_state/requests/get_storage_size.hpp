/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_SIZE
#define KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_SIZE

#include <jsonrpc-lean/request.h>

#include <optional>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::child_state::request {

  class GetStorageSize final {
   public:
    GetStorageSize(GetStorageSize const &) = delete;
    GetStorageSize &operator=(GetStorageSize const &) = delete;

    GetStorageSize(GetStorageSize &&) = default;
    GetStorageSize &operator=(GetStorageSize &&) = default;

    explicit GetStorageSize(std::shared_ptr<StateApi> api)
        : api_(std::move(api)){};
    ~GetStorageSize() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::optional<uint64_t>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    common::Buffer child_storage_key_;
    common::Buffer key_;
    std::optional<kagome::primitives::BlockHash> at_;
  };

}  // namespace kagome::api::child_state::request

#endif  // KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_STORAGE_SIZE

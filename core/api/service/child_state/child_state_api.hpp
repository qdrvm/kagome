/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SERVICE_CHILD_STATE_API
#define KAGOME_API_SERVICE_CHILD_STATE_API

#include <optional>
#include <vector>

#include "api/service/api_service.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"

namespace kagome::api {

  class ChildStateApi {
   public:
    virtual ~ChildStateApi() = default;

    virtual void setApiService(
        const std::shared_ptr<api::ApiService> &api_service) = 0;

    virtual outcome::result<std::vector<common::Buffer>> getKeys(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    virtual outcome::result<std::vector<common::Buffer>> getKeysPaged(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix,
        uint32_t keys_amount,
        const std::optional<common::Buffer> &prev_key_opt,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    virtual outcome::result<std::optional<common::Buffer>> getStorage(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    virtual outcome::result<std::optional<primitives::BlockHash>>
    getStorageHash(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    virtual outcome::result<std::optional<uint64_t>> getStorageSize(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SERVICE_CHILD_STATE_API

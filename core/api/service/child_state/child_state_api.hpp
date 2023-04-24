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

    /**
     * @brief Warning: This method is UNSAFE.
     * Returns the keys from the specified child storage.
     * The keys can also be filtered based on a prefix.
     * @param child_storage_key The child storage key.
     * @param prefix The prefix of the child storage keys to be filtered for.
     * Leave empty ("") to return all child storage keys.
     * @param block_hash_opt (OPTIONAL) The block hash indicating the state.
     * NULL implies the current state.
     * @return (OPTIONAL) Storage keys (Array)
     */
    virtual outcome::result<std::vector<common::Buffer>> getKeys(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    /**
     * @brief Warning: This method is UNSAFE.
     * Returns the keys from the specified child storage.
     * Paginated version of getKeys.
     * The keys can also be filtered based on a prefix.
     * @param child_storage_key The child storage key.
     * @param prefix The prefix of the child storage keys to be filtered for.
     * @param keys_amount Result page limit
     * @param prev_key_opt Last reported key
     * @param block_hash_opt (OPTIONAL) The block hash indicating the state.
     * NULL implies the current state.
     * @return (OPTIONAL) Storage keys (Array, up to keys_amount of size)
     */
    virtual outcome::result<std::vector<common::Buffer>> getKeysPaged(
        const common::Buffer &child_storage_key,
        const std::optional<common::Buffer> &prefix,
        uint32_t keys_amount,
        const std::optional<common::Buffer> &prev_key_opt,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    /**
     * @brief Returns a child storage entry.
     * @param child_storage_key The child storage key.
     * @param key The key within the child storage.
     * @param block_hash_opt (OPTIONAL) The block hash indicating the state.
     * NULL implies the current state.
     * @return (OPTIONAL) Storage data, if found.
     */
    virtual outcome::result<std::optional<common::Buffer>> getStorage(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    /**
     * @brief Returns the hash of a child storage entry.
     * @param child_storage_key The child storage key.
     * @param key The key within the child storage.
     * @param block_hash_opt (OPTIONAL) The block hash indicating the state.
     * NULL implies the current state.
     * @return (OPTIONAL) The hash of the child storage entry, if found.
     */
    virtual outcome::result<std::optional<primitives::BlockHash>>
    getStorageHash(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;

    /**
     * @brief Returns the size of a child storage entry.
     * @param child_storage_key The child storage key.
     * @param key The key within the child storage.
     * @param block_hash_opt (OPTIONAL) The block hash indicating the state.
     * NULL implies the current state.
     * @return (OPTIONAL) The size of the storage entry in bytes, if found.
     */
    virtual outcome::result<std::optional<uint64_t>> getStorageSize(
        const common::Buffer &child_storage_key,
        const common::Buffer &key,
        const std::optional<primitives::BlockHash> &block_hash_opt) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SERVICE_CHILD_STATE_API

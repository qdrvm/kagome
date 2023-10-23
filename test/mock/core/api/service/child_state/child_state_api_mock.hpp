/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "api/service/child_state/child_state_api.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {

  class ChildStateApiMock : public ChildStateApi {
   public:
    ~ChildStateApiMock() override = default;

    MOCK_METHOD(outcome::result<std::vector<common::Buffer>>,
                getKeys,
                (const common::Buffer &child_storage_key,
                 const std::optional<common::Buffer> &prefix,
                 const std::optional<primitives::BlockHash> &block_hash_opt),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<common::Buffer>>,
                getKeysPaged,
                (const common::Buffer &child_storage_key,
                 const std::optional<common::Buffer> &prefix,
                 uint32_t keys_amount,
                 const std::optional<common::Buffer> &prev_key_opt,
                 const std::optional<primitives::BlockHash> &block_hash_opt),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<common::Buffer>>,
                getStorage,
                (const common::Buffer &child_storage_key,
                 const common::Buffer &key,
                 const std::optional<primitives::BlockHash> &block_hash_opt),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<primitives::BlockHash>>,
                getStorageHash,
                (const common::Buffer &child_storage_key,
                 const common::Buffer &key,
                 const std::optional<primitives::BlockHash> &block_hash_opt),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<uint64_t>>,
                getStorageSize,
                (const common::Buffer &child_storage_key,
                 const common::Buffer &key,
                 const std::optional<primitives::BlockHash> &block_hash_opt),
                (const, override));
  };
}  // namespace kagome::api

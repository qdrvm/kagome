/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "api/service/state/state_api.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {

  class StateApiMock : public StateApi {
   public:
    ~StateApiMock() override = default;

    MOCK_METHOD(outcome::result<common::Buffer>,
                call,
                (std::string_view,
                 common::Buffer,
                 const std::optional<primitives::BlockHash> &),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<common::Buffer>>,
                getKeysPaged,
                (const std::optional<common::BufferView> &,
                 uint32_t,
                 const std::optional<common::BufferView> &,
                 const std::optional<primitives::BlockHash> &),
                (const, override));

    MOCK_METHOD(outcome::result<std::optional<common::Buffer>>,
                getStorage,
                (const common::BufferView &key),
                (const, override));

    outcome::result<std::optional<common::Buffer>> getStorage(
        const common::Buffer &key) const {
      return getStorage(common::BufferView{key});
    }

    MOCK_METHOD(outcome::result<std::optional<common::Buffer>>,
                getStorageAt,
                (const common::BufferView &key,
                 const primitives::BlockHash &at),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<StorageChangeSet>>,
                queryStorage,
                (gsl::span<const common::Buffer> keys,
                 const primitives::BlockHash &from,
                 std::optional<primitives::BlockHash> to),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<StorageChangeSet>>,
                queryStorageAt,
                (gsl::span<const common::Buffer> keys,
                 std::optional<primitives::BlockHash> at),
                (const, override));

    MOCK_METHOD(outcome::result<ReadProof>,
                getReadProof,
                (gsl::span<const common::Buffer> keys,
                 std::optional<primitives::BlockHash> at),
                (const, override));

    MOCK_METHOD(outcome::result<uint32_t>,
                subscribeStorage,
                (const std::vector<common::Buffer> &keys),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeStorage,
                (const std::vector<uint32_t> &subscription_id),
                (override));

    MOCK_METHOD(outcome::result<primitives::Version>,
                getRuntimeVersion,
                (const std::optional<primitives::BlockHash> &at),
                (const, override));

    MOCK_METHOD(outcome::result<uint32_t>,
                subscribeRuntimeVersion,
                (),
                (override));

    MOCK_METHOD(outcome::result<void>,
                unsubscribeRuntimeVersion,
                (uint32_t subscription_id),
                (override));

    MOCK_METHOD(outcome::result<std::string>, getMetadata, (), (override));

    MOCK_METHOD(outcome::result<std::string>,
                getMetadata,
                (std::string_view),
                (override));
  };
}  // namespace kagome::api

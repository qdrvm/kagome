/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STATE_API_MOCK_HPP
#define KAGOME_TEST_CORE_STATE_API_MOCK_HPP

#include <gmock/gmock.h>

#include "api/service/state/state_api.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {

  class StateApiMock : public StateApi {
   public:
    ~StateApiMock() override = default;

    MOCK_METHOD1(setApiService, void(const std::shared_ptr<api::ApiService> &));

    MOCK_CONST_METHOD4(getKeysPaged,
                       outcome::result<std::vector<common::Buffer>>(
                           const boost::optional<common::Buffer> &,
                           uint32_t,
                           const boost::optional<common::Buffer> &,
                           const boost::optional<primitives::BlockHash> &));

    MOCK_CONST_METHOD1(getStorage,
                       outcome::result<boost::optional<common::Buffer>>(
                           const common::Buffer &key));
    MOCK_CONST_METHOD2(getStorageAt,
                       outcome::result<boost::optional<common::Buffer>>(
                           const common::Buffer &key,
                           const primitives::BlockHash &at));

    MOCK_CONST_METHOD3(queryStorage,
                       outcome::result<std::vector<StorageChangeSet>>(
                           gsl::span<const common::Buffer> keys,
                           const primitives::BlockHash &from,
                           boost::optional<primitives::BlockHash> to));
    MOCK_CONST_METHOD2(queryStorageAt,
                       outcome::result<std::vector<StorageChangeSet>>(
                           gsl::span<const common::Buffer> keys,
                           boost::optional<primitives::BlockHash> at));

    MOCK_METHOD1(
        subscribeStorage,
        outcome::result<uint32_t>(std::vector<common::Buffer> const &keys));
    MOCK_METHOD1(
        unsubscribeStorage,
        outcome::result<bool>(const std::vector<uint32_t> &subscription_id));

    MOCK_CONST_METHOD1(getRuntimeVersion,
                       outcome::result<primitives::Version>(
                           boost::optional<primitives::BlockHash> const &at));
    MOCK_METHOD0(subscribeRuntimeVersion, outcome::result<uint32_t>());
    MOCK_METHOD1(unsubscribeRuntimeVersion,
                 outcome::result<void>(uint32_t subscription_id));

    MOCK_METHOD0(getMetadata, outcome::result<std::string>());
    MOCK_METHOD1(getMetadata, outcome::result<std::string>(std::string_view));
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_STATE_API_MOCK_HPP

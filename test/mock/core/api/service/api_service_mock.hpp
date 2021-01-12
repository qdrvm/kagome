/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SERVICE_MOCK_HPP
#define KAGOME_API_SERVICE_MOCK_HPP

#include <gmock/gmock.h>

#include "api/service/api_service.hpp"

namespace kagome::api {

  class ApiServiceMock : public ApiService {
   public:
    MOCK_METHOD0(prepare, bool());
    MOCK_METHOD0(start, bool());
    MOCK_METHOD0(stop, void());

    MOCK_METHOD1(
        subscribeSessionToKeys,
        outcome::result<uint32_t>(const std::vector<common::Buffer> &));
    MOCK_METHOD1(unsubscribeSessionFromIds,
                 outcome::result<bool>(
                     const std::vector<PubsubSubscriptionId> &));
    MOCK_METHOD0(subscribeFinalizedHeads,
                 outcome::result<PubsubSubscriptionId>());
    MOCK_METHOD1(unsubscribeFinalizedHeads,
                 outcome::result<bool>(PubsubSubscriptionId));
    MOCK_METHOD0(subscribeNewHeads, outcome::result<PubsubSubscriptionId>());
    MOCK_METHOD1(unsubscribeNewHeads,
                 outcome::result<bool>(PubsubSubscriptionId));
    MOCK_METHOD0(subscribeRuntimeVersion,
                 outcome::result<PubsubSubscriptionId>());
    MOCK_METHOD1(unsubscribeRuntimeVersion,
                 outcome::result<bool>(PubsubSubscriptionId));
    MOCK_METHOD1(subscribeForExtrinsicLifecycle,
                 outcome::result<PubsubSubscriptionId>(
                     const primitives::Transaction &));
    MOCK_METHOD1(unsubscribeFromExtrinsicLifecycle,
                 outcome::result<bool>(PubsubSubscriptionId));
  };

}  // namespace kagome::api

#endif  // KAGOME_API_SERVICE_MOCK_HPP

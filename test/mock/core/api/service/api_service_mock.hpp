/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "api/service/api_service.hpp"

namespace kagome::api {

  class ApiServiceMock : public ApiService {
   public:
    MOCK_METHOD(bool, prepare, (), (override));

    MOCK_METHOD(bool, start, (), (override));

    MOCK_METHOD(void, stop, (), (override));

    MOCK_METHOD(outcome::result<uint32_t>,
                subscribeSessionToKeys,
                (const std::vector<common::Buffer> &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeSessionFromIds,
                (const std::vector<PubsubSubscriptionId> &),
                (override));

    MOCK_METHOD(outcome::result<PubsubSubscriptionId>,
                subscribeFinalizedHeads,
                (),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeFinalizedHeads,
                (PubsubSubscriptionId),
                (override));

    MOCK_METHOD(outcome::result<PubsubSubscriptionId>,
                subscribeNewHeads,
                (),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeNewHeads,
                (PubsubSubscriptionId),
                (override));

    MOCK_METHOD(outcome::result<PubsubSubscriptionId>,
                subscribeRuntimeVersion,
                (),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeRuntimeVersion,
                (PubsubSubscriptionId),
                (override));

    MOCK_METHOD(outcome::result<PubsubSubscriptionId>,
                subscribeForExtrinsicLifecycle,
                (const primitives::Transaction::Hash &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unsubscribeFromExtrinsicLifecycle,
                (PubsubSubscriptionId),
                (override));
  };

}  // namespace kagome::api

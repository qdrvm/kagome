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

    MOCK_CONST_METHOD1(
        getStorage, outcome::result<common::Buffer>(const common::Buffer &key));
    MOCK_CONST_METHOD2(
        getStorage,
        outcome::result<common::Buffer>(const common::Buffer &key,
                                        const primitives::BlockHash &at));
    MOCK_CONST_METHOD1(getRuntimeVersion,
                       outcome::result<primitives::Version>(
                           boost::optional<primitives::BlockHash> const &at));
    MOCK_METHOD1(
        subscribeStorage,
        outcome::result<uint32_t>(std::vector<common::Buffer> const &keys));
    MOCK_METHOD1(
        unsubscribeStorage,
        outcome::result<void>(const std::vector<uint32_t> &subscription_id));
  };
}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_STATE_API_MOCK_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_API_SERVICE_AUTHOR_AUTHOR_API_MOCK_HPP
#define KAGOME_TEST_CORE_API_SERVICE_AUTHOR_AUTHOR_API_MOCK_HPP

#include <gmock/gmock.h>

#include "api/service/author/author_api.hpp"

namespace kagome::api {

  class AuthorApiMock : public AuthorApi {
   public:
    ~AuthorApiMock() override = default;

    MOCK_METHOD1(submitExtrinsic, outcome::result<Hash256>(const Extrinsic &));

    MOCK_METHOD1(submitAndWatchExtrinsic, outcome::result<SubscriptionId>(Extrinsic));

    MOCK_METHOD1(unwatchExtrinsic, outcome::result<bool>(SubscriptionId));

    MOCK_METHOD0(pendingExtrinsics, outcome::result<std::vector<Extrinsic>>());

    MOCK_METHOD1(removeExtrinsic,
                 outcome::result<std::vector<Hash256>>(
                     const std::vector<ExtrinsicKey> &));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_API_SERVICE_AUTHOR_AUTHOR_API_MOCK_HPP

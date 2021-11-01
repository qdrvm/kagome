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

    MOCK_METHOD(void,
                setApiService,
                (const std::shared_ptr<api::ApiService> &),
                (override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                submitExtrinsic,
                (const Extrinsic &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                insertKey,
                (crypto::KeyTypeId,
                 const gsl::span<const uint8_t> &,
                 const gsl::span<const uint8_t> &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                hasSessionKeys,
                (const gsl::span<const uint8_t> &),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                hasKey,
                (const gsl::span<const uint8_t> &public_key,
                 crypto::KeyTypeId key_type),
                (override));

    MOCK_METHOD(outcome::result<SubscriptionId>,
                submitAndWatchExtrinsic,
                (Extrinsic),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                unwatchExtrinsic,
                (SubscriptionId),
                (override));

    MOCK_METHOD(outcome::result<std::vector<Extrinsic>>,
                pendingExtrinsics,
                (),
                (override));

    MOCK_METHOD(outcome::result<std::vector<Extrinsic>>,
                removeExtrinsic,
                (const std::vector<ExtrinsicKey> &),
                (override));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_API_SERVICE_AUTHOR_AUTHOR_API_MOCK_HPP

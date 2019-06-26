/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP
#define KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP

#include "testutil/vector_printer.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "api/extrinsic/impl/extrinsic_api_impl.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {

  class ExtrinsicApiMock : public ExtrinsicApi {
   public:
    ~ExtrinsicApiMock() override = default;

    MOCK_METHOD1(submit_extrinsic, outcome::result<Hash256>(const Extrinsic &));

    MOCK_METHOD0(pending_extrinsics, outcome::result<std::vector<Extrinsic>>());

    MOCK_METHOD1(remove_extrinsic,
                 outcome::result<std::vector<Hash256>>(
                     const std::vector<ExtrinsicKey> &));

    MOCK_METHOD3(watch_extrinsic,
                 void(const Metadata &, const Subscriber &, const Buffer &));

    MOCK_METHOD2(unwatch_extrinsic,
                 outcome::result<bool>(const std::optional<Metadata> &,
                                       const SubscriptionId &));
  };

}  // namespace kagome::service

#endif  // KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP

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

    MOCK_METHOD1(submitExtrinsic, outcome::result<Hash256>(const Extrinsic &));

    MOCK_METHOD0(pendingExtrinsics, outcome::result<std::vector<Extrinsic>>());

    MOCK_METHOD1(removeExtrinsic,
                 outcome::result<std::vector<Hash256>>(
                     const std::vector<ExtrinsicKey> &));
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP

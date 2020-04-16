/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP
#define KAGOME_TEST_CORE_EXTRINSIC_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_MOCK_HPP

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

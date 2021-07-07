/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_PROVIDER_MOCK_HPP

#include "runtime/core_api_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class CoreApiProviderMock : public CoreApiFactory {
   public:
    MOCK_CONST_METHOD2(
        make,
        std::unique_ptr<Core>(std::shared_ptr<const crypto::Hasher> hasher,
                              gsl::span<uint8_t> runtime_code));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_PROVIDER_MOCK_HPP

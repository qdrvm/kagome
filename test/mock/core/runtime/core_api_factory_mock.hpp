/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP

#include "runtime/core_api_factory_impl.hpp"

#include <gmock/gmock.h>

#include "runtime/executor.hpp"

namespace kagome::runtime {

  class CoreApiFactoryMock : public CoreApiFactory {
   public:
    MOCK_CONST_METHOD2(
        make,
        std::unique_ptr<Core>(std::shared_ptr<const crypto::Hasher> hasher,
                              const std::vector<uint8_t> &runtime_code));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP

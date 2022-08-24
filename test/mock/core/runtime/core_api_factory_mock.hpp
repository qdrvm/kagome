/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP

#include "runtime/core_api_factory.hpp"

#include <gmock/gmock.h>

#include "runtime/common/executor.hpp"
#include "runtime/runtime_api/core.hpp"

namespace kagome::runtime {

  class CoreApiFactoryMock : public CoreApiFactory {
   public:
    MOCK_METHOD(std::unique_ptr<Core>,
                make,
                (std::shared_ptr<const crypto::Hasher> hasher,
                 const std::vector<uint8_t> &runtime_code),
                (const, override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_API_FACTORY_MOCK_HPP

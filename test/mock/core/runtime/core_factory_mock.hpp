/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_FACTORY_MOCK_HPP
#define KAGOME_CORE_FACTORY_MOCK_HPP

#include "runtime/binaryen/core_api_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class CoreFactoryMock : public CoreApiFactory {
   public:
    ~CoreFactoryMock() override = default;

    MOCK_CONST_METHOD2(
        make,
        std::unique_ptr<Core>(std::shared_ptr<const crypto::Hasher> hasher,
                              const std::vector<uint8_t> &runtime_code));
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_FACTORY_MOCK_HPP

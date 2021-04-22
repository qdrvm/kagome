/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_FACTORY_MOCK_HPP
#define KAGOME_CORE_FACTORY_MOCK_HPP

#include "runtime/binaryen/core_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::binaryen {

  class CoreFactoryMock : public CoreFactory {
   public:
    ~CoreFactoryMock() override = default;

    MOCK_METHOD2(
        createWithCode,
        std::unique_ptr<Core>(
            std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory,
            std::shared_ptr<RuntimeCodeProvider> wasm_provider));
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_FACTORY_MOCK_HPP

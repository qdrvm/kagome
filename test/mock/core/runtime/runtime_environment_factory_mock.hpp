/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP

#include "runtime/runtime_environment_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimeEnvironmentTemplateMock
      : public RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate {
    MOCK_METHOD1(
        setState,
        RuntimeEnvironmentTemplate &(const primitives::BlockInfo &state));

    MOCK_METHOD0(persistent, RuntimeEnvironmentTemplate &());

    MOCK_METHOD1(
        on_state_destruction,
        RuntimeEnvironmentTemplate &(std::function<void()> callback));

    MOCK_METHOD0(make, outcome::result<RuntimeEnvironment> ());
  };

  class RuntimeEnvironmentFactoryMock : public RuntimeEnvironmentFactory {
   public:

  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP

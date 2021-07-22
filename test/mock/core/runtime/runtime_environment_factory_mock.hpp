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
   public:
    RuntimeEnvironmentTemplateMock(
        std::weak_ptr<RuntimeEnvironmentFactory> parent)
        : RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate{
            std::move(parent)} {}

    MOCK_METHOD1(
        setState,
        RuntimeEnvironmentTemplate &(const primitives::BlockInfo &state));

    MOCK_METHOD0(persistent, RuntimeEnvironmentTemplate &());

    MOCK_METHOD1(on_state_destruction,
                 RuntimeEnvironmentTemplate &(std::function<void()> callback));

    MOCK_METHOD0(make, outcome::result<std::unique_ptr<RuntimeEnvironment>>());
  };

  class RuntimeEnvironmentFactoryMock : public RuntimeEnvironmentFactory {
   public:
    RuntimeEnvironmentFactoryMock(
        std::shared_ptr<TrieStorageProvider> storage_provider,
        std::shared_ptr<host_api::HostApi> host_api,
        std::shared_ptr<MemoryProvider> memory_provider,
        std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
        : RuntimeEnvironmentFactory{std::move(storage_provider),
                                    std::move(host_api),
                                    std::move(memory_provider),
                                    std::move(code_provider),
                                    std::move(module_repo),
                                    std::move(header_repo)} {}

    std::unique_ptr<RuntimeEnvironmentTemplate> start() override {
      return std::make_unique<RuntimeEnvironmentTemplateMock>(weak_from_this());
    }
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP

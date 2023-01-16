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
        std::weak_ptr<RuntimeEnvironmentFactory> parent,
        const primitives::BlockInfo &blockchain_state,
        const storage::trie::RootHash &storage_state)
        : RuntimeEnvironmentFactory::RuntimeEnvironmentTemplate{
            std::move(parent), blockchain_state, storage_state} {}

    MOCK_METHOD(RuntimeEnvironmentTemplate &,
                setState,
                (const primitives::BlockInfo &state),
                ());

    MOCK_METHOD(RuntimeEnvironmentTemplate &, persistent, (), (override));

    MOCK_METHOD(RuntimeEnvironmentTemplate &,
                on_state_destruction,
                (std::function<void()> callback));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeEnvironment>>,
                make,
                (OnDbRead),
                (override));
  };

  class RuntimeEnvironmentFactoryMock : public RuntimeEnvironmentFactory {
   public:
    RuntimeEnvironmentFactoryMock(
        std::shared_ptr<const runtime::RuntimeCodeProvider> code_provider,
        std::shared_ptr<ModuleRepository> module_repo,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
        : RuntimeEnvironmentFactory{std::move(code_provider),
                                    std::move(module_repo),
                                    std::move(header_repo)} {}

    MOCK_METHOD(std::unique_ptr<RuntimeEnvironmentTemplate>,
                start,
                (const primitives::BlockInfo &blockchain_state,
                 const storage::trie::RootHash &storage_state),
                (const, override));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeEnvironmentTemplate>>,
                start,
                (const primitives::BlockHash &blockchain_state),
                (const, override));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeEnvironmentTemplate>>,
                start,
                (),
                (const, override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_ENVIRONMENT_FACTORY_MOCK_HPP

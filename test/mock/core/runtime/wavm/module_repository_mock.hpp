/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_MODULE_REPOSITORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_MODULE_REPOSITORY_MOCK_HPP

#include "runtime/wavm/module_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime::wavm {

  class ModuleRepositoryMock final : public ModuleRepository {
   public:
    MOCK_METHOD2(getInstanceAt,
                  outcome::result<std::shared_ptr<ModuleInstance>>(
                      std::shared_ptr<RuntimeCodeProvider> code_provider,
                      const primitives::BlockInfo &block));

    MOCK_METHOD1(getInstanceAtLatest,
                  outcome::result<std::shared_ptr<ModuleInstance>>(
                      std::shared_ptr<RuntimeCodeProvider> code_provider));

    MOCK_METHOD1(loadFrom,
                  outcome::result<std::unique_ptr<Module>>(
                      gsl::span<const uint8_t> byte_code));
  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_MODULE_REPOSITORY_MOCK_HPP

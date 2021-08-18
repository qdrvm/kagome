/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP

#include "runtime/module_repository.hpp"

#include <gmock/gmock.h>

#include "runtime/instance_environment.hpp"

namespace kagome::runtime {

  class ModuleRepositoryMock final : public ModuleRepository {
   public:
    MOCK_METHOD2(getInstanceAt,
                 outcome::result<std::pair<std::shared_ptr<ModuleInstance>,
                                           InstanceEnvironment>>(
                     std::shared_ptr<const RuntimeCodeProvider> code_provider,
                     const primitives::BlockInfo &block));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP

#include "runtime/module_repository.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ModuleRepositoryMock final : public ModuleRepository {
   public:
    MOCK_METHOD2(getInstanceAt,
                 outcome::result<std::shared_ptr<ModuleInstance>>(
                     std::shared_ptr<const RuntimeCodeProvider> code_provider,
                     const primitives::BlockInfo &block));

  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_MODULE_REPOSITORY_MOCK_HPP

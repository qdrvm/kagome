/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_repository.hpp"

#include <gmock/gmock.h>

#include "runtime/instance_environment.hpp"

namespace kagome::runtime {

  class ModuleRepositoryMock final : public ModuleRepository {
   public:
    MOCK_METHOD(outcome::result<std::shared_ptr<ModuleInstance>>,
                getInstanceAt,
                (const primitives::BlockInfo &block,
                 const storage::trie::RootHash &state_root),
                (override));
  };

}  // namespace kagome::runtime

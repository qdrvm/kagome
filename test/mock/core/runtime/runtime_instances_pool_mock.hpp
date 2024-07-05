/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_instances_pool.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimeInstancesPoolMock : public RuntimeInstancesPool {
   public:
    MOCK_METHOD(outcome::result<std::shared_ptr<ModuleInstance>>,
                instantiateFromCode,
                (const CodeHash &code_hash,
                 common::BufferView code_zstd,
                 const RuntimeContext::ContextParams &config));

    MOCK_METHOD(void,
                release,
                (const TrieHash &state,
                 std::shared_ptr<ModuleInstance> &&instance));
  };

}  // namespace kagome::runtime

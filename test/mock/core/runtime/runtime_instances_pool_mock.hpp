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
                 const Config &config));

    MOCK_METHOD(outcome::result<std::shared_ptr<ModuleInstance>>,
                instantiateFromState,
                (const TrieHash &state, const Config &config));

    MOCK_METHOD(void,
                release,
                (const TrieHash &state,
                 std::shared_ptr<ModuleInstance> &&instance));

    MOCK_METHOD(std::optional<std::shared_ptr<const Module>>,
                getModule,
                (const TrieHash &state));

    MOCK_METHOD(void,
                putModule,
                (const TrieHash &state, std::shared_ptr<Module> module));
  };

}  // namespace kagome::runtime

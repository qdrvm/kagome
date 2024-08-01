/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_context.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   */
  class RuntimeInstancesPool {
   public:
    static constexpr size_t DEFAULT_MODULES_CACHE_SIZE = 2;

    using CodeHash = common::Hash256;
    using GetCode = std::function<RuntimeCodeProvider::Result()>;

    virtual ~RuntimeInstancesPool() = default;

    virtual outcome::result<std::shared_ptr<ModuleInstance>>
    instantiateFromCode(const CodeHash &code_hash,
                        const GetCode &get_code,
                        const RuntimeContext::ContextParams &config) = 0;
  };

}  // namespace kagome::runtime

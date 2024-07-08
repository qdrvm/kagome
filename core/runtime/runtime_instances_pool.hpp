/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   */
  class RuntimeInstancesPool {
   public:
    static constexpr size_t DEFAULT_MODULES_CACHE_SIZE = 2;

    using CodeHash = storage::trie::RootHash;

    virtual ~RuntimeInstancesPool() = default;

    virtual outcome::result<std::shared_ptr<ModuleInstance>>
    instantiateFromCode(const CodeHash &code_hash,
                        common::BufferView code_zstd,
                        const RuntimeContext::ContextParams &config) = 0;
  };

}  // namespace kagome::runtime

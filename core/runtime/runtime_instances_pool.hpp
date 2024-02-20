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

    using TrieHash = storage::trie::RootHash;
    using CodeHash = storage::trie::RootHash;

    virtual ~RuntimeInstancesPool() = default;

    virtual outcome::result<std::shared_ptr<ModuleInstance>>
    instantiateFromCode(const CodeHash &code_hash,
                        common::BufferView code_zstd,
                        const RuntimeContext::ContextParams &config) = 0;

    /**
     * @brief Instantiate new or reuse existing ModuleInstance for the provided
     * state.
     *
     * @param state - the merkle trie root of the state containing the code of
     * the runtime module we are acquiring an instance of.
     * @return pointer to the acquired ModuleInstance if success. Error
     * otherwise.
     */
    virtual outcome::result<std::shared_ptr<ModuleInstance>>
    instantiateFromState(const TrieHash &state,
                         const RuntimeContext::ContextParams &config) = 0;
    /**
     * @brief Releases the module instance (returns it to the pool)
     *
     * @param state - the merkle trie root of the state containing the runtime
     * module code we are releasing an instance of.
     * @param instance - instance to be released.
     */
    virtual void release(const TrieHash &state,
                         std::shared_ptr<ModuleInstance> &&instance) = 0;

    /**
     * @brief Get the module for state from internal cache
     *
     * @param state - the state containing the module's code.
     * @return Module if any, nullopt otherwise
     */
    virtual std::optional<std::shared_ptr<const Module>> getModule(
        const TrieHash &state) = 0;

    /**
     * @brief Puts new module into internal cache
     *
     * @param state - storage hash of the block containing the code of the
     * module
     * @param module - new module pointer
     */
    virtual void putModule(const TrieHash &state,
                           std::shared_ptr<Module> module) = 0;
  };

}  // namespace kagome::runtime

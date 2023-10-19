/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP
#define KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP

#include "runtime/module_repository.hpp"

#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <stack>
#include <unordered_set>

#include "runtime/module_factory.hpp"
#include "utils/lru.hpp"

namespace kagome::runtime {

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   *
   */
  class RuntimeInstancesPool final
      : public std::enable_shared_from_this<RuntimeInstancesPool> {
    using ModuleInstancePool = std::stack<std::shared_ptr<ModuleInstance>>;

    static constexpr size_t DEFAULT_MODULES_CACHE_SIZE = 2;

   public:
    using TrieHash = storage::trie::RootHash;
    using CodeHash = storage::trie::RootHash;

    RuntimeInstancesPool(std::shared_ptr<ModuleFactory> module_factory,
                         size_t capacity = DEFAULT_MODULES_CACHE_SIZE);

    outcome::result<std::shared_ptr<ModuleInstance>, CompilationError>
    instantiateFromCode(const CodeHash &code_hash,
                        common::BufferView code_zstd);

    /**
     * @brief Instantiate new or reuse existing ModuleInstance for the provided
     * state.
     *
     * @param state - the merkle trie root of the state containing the code of
     * the runtime module we are acquiring an instance of.
     * @return pointer to the acquired ModuleInstance if success. Error
     * otherwise.
     */
    outcome::result<std::shared_ptr<ModuleInstance>> instantiateFromState(
        const TrieHash &state);
    /**
     * @brief Releases the module instance (returns it to the pool)
     *
     * @param state - the merkle trie root of the state containing the runtime
     * module code we are releasing an instance of.
     * @param instance - instance to be released.
     */
    void release(const TrieHash &state,
                 std::shared_ptr<ModuleInstance> &&instance);

    /**
     * @brief Get the module for state from internal cache
     *
     * @param state - the state containing the module's code.
     * @return Module if any, nullopt otherwise
     */
    std::optional<std::shared_ptr<const Module>> getModule(
        const TrieHash &state);

    /**
     * @brief Puts new module into internal cache
     *
     * @param state - storage hash of the block containing the code of the
     * module
     * @param module - new module pointer
     */
    void putModule(const TrieHash &state, std::shared_ptr<Module> module);

   private:
    struct InstancePool {
      std::shared_ptr<const Module> module;
      std::vector<std::shared_ptr<ModuleInstance>> instances;

      std::shared_ptr<ModuleInstance> instantiate(
          std::unique_lock<std::shared_mutex> &lock);
    };

    outcome::result<std::reference_wrapper<InstancePool>, CompilationError>
    tryCompileModule(const CodeHash &code_hash, common::BufferView code_zstd);

    std::shared_ptr<ModuleFactory> module_factory_;

    std::shared_mutex pools_mtx_;
    Lru<common::Hash256, InstancePool> pools_;

    struct WaitEntry {
      WaitEntry() = default;

      std::shared_mutex mutex;
      std::atomic_int waiting_threads_num = 0;
      bool compilation_done = false;
    };
    std::shared_mutex wait_queue_mtx_;
    std::unordered_map<CodeHash, WaitEntry> wait_queue_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP

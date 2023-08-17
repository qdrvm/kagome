/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP
#define KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP

#include "runtime/module_repository.hpp"

#include <mutex>
#include <stack>

#include "utils/lru.hpp"

namespace kagome::runtime {
  class ModuleFactory;

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   *
   */
  class RuntimeInstancesPool final
      : public std::enable_shared_from_this<RuntimeInstancesPool> {
    using ModuleInstancePool = std::stack<std::shared_ptr<ModuleInstance>>;

   public:
    using RootHash = storage::trie::RootHash;

    RuntimeInstancesPool();

    RuntimeInstancesPool(std::shared_ptr<ModuleFactory> module_factory,
                         size_t capacity);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiate(
        const RootHash &code_hash, common::BufferView code_zstd);

    /**
     * @brief Instantiate new or reuse existing ModuleInstance for the provided
     * state.
     *
     * @param state - the merkle trie root of the state containing the code of
     * the runtime module we are acquiring an instance of.
     * @return pointer to the acquired ModuleInstance if success. Error
     * otherwise.
     */
    outcome::result<std::shared_ptr<ModuleInstance>> tryAcquire(
        const RootHash &state);
    /**
     * @brief Releases the module instance (returns it to the pool)
     *
     * @param state - the merkle trie root of the state containing the runtime
     * module code we are releasing an instance of.
     * @param instance - instance to be released.
     */
    void release(const RootHash &state,
                 std::shared_ptr<ModuleInstance> &&instance);

    /**
     * @brief Get the module for state from internal cache
     *
     * @param state - the state containing the module's code.
     * @return Module if any, nullopt otherwise
     */
    std::optional<std::shared_ptr<const Module>> getModule(
        const RootHash &state);

    /**
     * @brief Puts new module into internal cache
     *
     * @param state - runtime block, by its root hash
     * @param module - new module pointer
     */
    void putModule(const RootHash &state, std::shared_ptr<Module> module);

   private:
    struct Entry {
      std::shared_ptr<const Module> module;
      std::vector<std::shared_ptr<ModuleInstance>> instances;

      outcome::result<std::shared_ptr<ModuleInstance>> instantiate(
          std::unique_lock<std::mutex> &lock);
    };

    std::shared_ptr<ModuleFactory> module_factory_;

    std::mutex mt_;
    static constexpr size_t MODULES_CACHE_SIZE = 2;
    Lru<common::Hash256, Entry> pools_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_INSTANCES_POOL_HPP

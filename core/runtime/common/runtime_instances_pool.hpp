/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_instances_pool.hpp"

#include <boost/di.hpp>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

#include "runtime/module_factory.hpp"
#include "utils/lru.hpp"

namespace kagome::runtime {
  class InstrumentWasm;

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   */
  class RuntimeInstancesPoolImpl final
      : public RuntimeInstancesPool,
        public std::enable_shared_from_this<RuntimeInstancesPoolImpl> {
   public:
    explicit RuntimeInstancesPoolImpl(
        std::shared_ptr<ModuleFactory> module_factory,
        std::shared_ptr<InstrumentWasm> instrument,
        size_t capacity = DEFAULT_MODULES_CACHE_SIZE);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiateFromCode(
        const CodeHash &code_hash,
        common::BufferView code_zstd,
        const RuntimeContext::ContextParams &config) override;

    /**
     * @brief Releases the module instance (returns it to the pool)
     *
     * @param state - the merkle trie root of the state containing the runtime
     * module code we are releasing an instance of.
     * @param instance - instance to be released.
     */
    void release(const CodeHash &code_hash,
                 const RuntimeContext::ContextParams &config,
                 std::shared_ptr<ModuleInstance> &&instance);

   private:
    using Key = std::tuple<common::Hash256, RuntimeContext::ContextParams>;
    struct InstancePool {
      std::shared_ptr<const Module> module;
      std::vector<std::shared_ptr<ModuleInstance>> instances;

      outcome::result<std::shared_ptr<ModuleInstance>> instantiate(
          std::unique_lock<std::mutex> &lock);
    };

    using CompilationResult =
        outcome::result<std::shared_ptr<const Module>, CompilationError>;
    CompilationResult tryCompileModule(
        const CodeHash &code_hash,
        common::BufferView code_zstd,
        const RuntimeContext::ContextParams &config);

    std::shared_ptr<ModuleFactory> module_factory_;
    std::shared_ptr<InstrumentWasm> instrument_;

    std::mutex pools_mtx_;
    Lru<Key, InstancePool> pools_;

    mutable std::mutex compiling_modules_mtx_;
    std::unordered_map<Key, std::shared_future<CompilationResult>>
        compiling_modules_;
  };

}  // namespace kagome::runtime

template <>
struct boost::di::ctor_traits<kagome::runtime::RuntimeInstancesPoolImpl> {
  BOOST_DI_INJECT_TRAITS(std::shared_ptr<kagome::runtime::ModuleFactory>,
                         std::shared_ptr<kagome::runtime::InstrumentWasm>);
};

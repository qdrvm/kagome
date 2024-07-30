/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/optref.hpp"
#include "runtime/runtime_instances_pool.hpp"

#include <boost/di.hpp>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>

#include "runtime/module_factory.hpp"
#include "utils/lru.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::runtime {
  class WasmInstrumenter;

  /**
   * @brief Pool of runtime instances - per state. Encapsulates modules cache.
   */
  class RuntimeInstancesPoolImpl final
      : public RuntimeInstancesPool,
        public std::enable_shared_from_this<RuntimeInstancesPoolImpl> {
   public:
    explicit RuntimeInstancesPoolImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<ModuleFactory> module_factory,
        std::shared_ptr<WasmInstrumenter> instrument,
        size_t capacity = DEFAULT_MODULES_CACHE_SIZE);

    outcome::result<std::shared_ptr<ModuleInstance>> instantiateFromCode(
        const CodeHash &code_hash,
        const GetCode &get_code,
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

    std::filesystem::path getCachePath(
        const CodeHash &code_hash,
        const RuntimeContext::ContextParams &config) const;

    outcome::result<void> precompile(
        const CodeHash &code_hash,
        const GetCode &get_code,
        const RuntimeContext::ContextParams &config);

    std::optional<std::shared_ptr<const Module>> getModule(
        const CodeHash &code_hash, const RuntimeContext::ContextParams &config);

   private:
    using Key = std::tuple<common::Hash256, RuntimeContext::ContextParams>;
    struct InstancePool {
      std::shared_ptr<const Module> module;
      std::vector<std::shared_ptr<ModuleInstance>> instances;

      outcome::result<std::shared_ptr<ModuleInstance>> instantiate(
          std::unique_lock<std::mutex> &lock);
    };

    outcome::result<std::reference_wrapper<InstancePool>> getPool(
        std::unique_lock<std::mutex> &lock,
        const CodeHash &code_hash,
        const GetCode &get_code,
        const RuntimeContext::ContextParams &config);

    using CompilationResult = CompilationOutcome<std::shared_ptr<const Module>>;
    CompilationResult tryCompileModule(
        const CodeHash &code_hash,
        const GetCode &get_code,
        const RuntimeContext::ContextParams &config);

    std::filesystem::path cache_dir_;
    std::shared_ptr<ModuleFactory> module_factory_;
    std::shared_ptr<WasmInstrumenter> instrument_;

    std::mutex pools_mtx_;
    Lru<Key, InstancePool> pools_;

    mutable std::mutex compiling_modules_mtx_;
    std::unordered_map<Key, std::shared_future<CompilationResult>>
        compiling_modules_;
  };

}  // namespace kagome::runtime

template <>
struct boost::di::ctor_traits<kagome::runtime::RuntimeInstancesPoolImpl> {
  BOOST_DI_INJECT_TRAITS(const kagome::application::AppConfiguration &,
                         std::shared_ptr<kagome::runtime::ModuleFactory>,
                         std::shared_ptr<kagome::runtime::WasmInstrumenter>);
};

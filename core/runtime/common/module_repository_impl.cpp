/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/module_repository_impl.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_code_provider.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {

  ModuleRepositoryImpl::ModuleRepositoryImpl(
      std::shared_ptr<RuntimeUpgradeTracker> runtime_upgrade_tracker,
      std::shared_ptr<const ModuleFactory> module_factory,
      std::shared_ptr<SingleModuleCache> last_compiled_module)
      : runtime_upgrade_tracker_{std::move(runtime_upgrade_tracker)},
        module_factory_{std::move(module_factory)},
        last_compiled_module_{std::move(last_compiled_module)},
        logger_{log::createLogger("Module Repository", "runtime")} {
    BOOST_ASSERT(runtime_upgrade_tracker_);
    BOOST_ASSERT(module_factory_);
    BOOST_ASSERT(last_compiled_module_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  ModuleRepositoryImpl::getInstanceAt(
      std::shared_ptr<const RuntimeCodeProvider> code_provider,
      const primitives::BlockInfo &block,
      const primitives::BlockHeader &header) {
    KAGOME_PROFILE_START(code_retrieval)
    OUTCOME_TRY(state, runtime_upgrade_tracker_->getLastCodeUpdateState(block));
    KAGOME_PROFILE_END(code_retrieval)

    KAGOME_PROFILE_START(module_retrieval) {
      // Add compiled module if any
      if (auto module = last_compiled_module_->try_extract();
          module.has_value()) {
        BOOST_VERIFY(runtime_instances_pool_.put_module(state, module.value()));
      }

      // Compile new module if required
      if (auto opt_module = runtime_instances_pool_.get_module(state);
          !opt_module.has_value()) {
        SL_DEBUG(
            logger_, "Runtime module cache miss for state {}", state.toHex());
        auto code = code_provider->getCodeAt(state);
        if (not code.has_value()) {
          code = code_provider->getCodeAt(header.state_root);
        }
        if (not code.has_value()) {
          return code.as_failure();
        }
        OUTCOME_TRY(new_module, module_factory_->make(code.value()));
        BOOST_VERIFY(
            runtime_instances_pool_.put_module(state, std::move(new_module)));
      }
    }

    // Try acquire instance (instantiate if needed)
    OUTCOME_TRY(runtime_instance, runtime_instances_pool_.try_acquire(state));
    KAGOME_PROFILE_END(module_retrieval)

    return runtime_instance;
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::try_acquire(
      const RuntimeInstancesPool::RootHash &state) {
    std::scoped_lock guard{mt_};
    auto tid = soralog::util::getThreadNumber();
    auto &pool = pools_[state];

    // if used instance requested, just return it - don't add to TLS
    if (auto inst_it = pool.find(tid); inst_it != pool.end()) {
      return inst_it->second;
    }

    // fetch unused, mark and return
    // if pool is empty, instantiate, mark as used and return
    auto node = pool.extract(POOL_FREE_INSTANCE_ID);
    std::shared_ptr<ModuleInstance> module_instance;
    if (not node) {
      auto opt_module = modules_.get(state);
      BOOST_ASSERT(opt_module.has_value());
      auto module = opt_module.value();
      OUTCOME_TRY(new_module_instance, module.get()->instantiate());
      module_instance = new_module_instance;
      pool.emplace(tid, std::move(new_module_instance));
    } else {
      node.key() = tid;
      module_instance = node.mapped();
      pool.insert(std::move(node));
    }
    auto borrowed_instance = std::make_shared<BorrowedRuntimeInstance>(
        module_instance, [this, state]() { release(state); });
    module_instance->post_instantiate(std::move(borrowed_instance));
    return module_instance;
  }

  void RuntimeInstancesPool::release(
      const RuntimeInstancesPool::RootHash &state) {
    std::lock_guard guard{mt_};
    auto tid = soralog::util::getThreadNumber();
    auto &pool = pools_[state];
    fmt::print("released tid {} in pool size {}\n", tid, pool.size());
    // if used instance found, release
    auto node = pool.extract(tid);
    BOOST_ASSERT(node);
    node.key() = POOL_FREE_INSTANCE_ID;
    pool.insert(std::move(node));
  }
  std::optional<std::shared_ptr<Module>> RuntimeInstancesPool::get_module(
      const RuntimeInstancesPool::RootHash &state) {
    std::lock_guard guard{mt_};
    return modules_.get(state);
  }
  bool RuntimeInstancesPool::put_module(
      const RuntimeInstancesPool::RootHash &state,
      std::shared_ptr<Module> module) {
    std::lock_guard guard{mt_};
    return modules_.put(state, std::move(module));
  }
}  // namespace kagome::runtime

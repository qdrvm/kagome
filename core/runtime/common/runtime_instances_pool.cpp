

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_instances_pool.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_upgrade_tracker.hpp"

namespace kagome::runtime {
  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::tryAcquire(
      const RuntimeInstancesPool::RootHash &state) {
    std::scoped_lock guard{mt_};
    auto &pool = pools_[state];

    if (not pool.empty()) {
      auto top = std::move(pool.top());
      pool.pop();
      return top;
    }

    auto opt_module = modules_.get(state);
    BOOST_ASSERT(opt_module.has_value());
    auto module = opt_module.value();
    OUTCOME_TRY(instance, module.get()->instantiate());

    return std::move(instance);
  }

  void RuntimeInstancesPool::release(
      const RuntimeInstancesPool::RootHash &state,
      std::shared_ptr<ModuleInstance> &&instance) {
    std::lock_guard guard{mt_};
    auto &pool = pools_[state];

    pool.emplace(std::move(instance));
  }
  std::optional<std::shared_ptr<Module>> RuntimeInstancesPool::getModule(
      const RuntimeInstancesPool::RootHash &state) {
    std::lock_guard guard{mt_};
    return modules_.get(state);
  }
  void RuntimeInstancesPool::putModule(
      const RuntimeInstancesPool::RootHash &state,
      std::shared_ptr<Module> module) {
    std::lock_guard guard{mt_};
    modules_.put(state, std::move(module));
  }
}  // namespace kagome::runtime

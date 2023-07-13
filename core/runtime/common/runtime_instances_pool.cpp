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
  /**
   * @brief Wrapper type over sptr<ModuleInstance>. Allows to return instance
   * back to the ModuleInstancePool upon destruction of
   * BorrowedInstance.
   */
  class BorrowedInstance : public ModuleInstance {
   public:
    BorrowedInstance(std::weak_ptr<RuntimeInstancesPool> pool,
                     const RuntimeInstancesPool::RootHash &state,
                     std::shared_ptr<ModuleInstance> instance)
        : pool_{std::move(pool)},
          state_{state},
          instance_{std::move(instance)} {}
    ~BorrowedInstance() {
      if (auto pool = pool_.lock()) {
        pool->release(state_, std::move(instance_));
      }
    }

    const common::Hash256 &getCodeHash() const override {
      return instance_->getCodeHash();
    }

    outcome::result<PtrSize> callExportFunction(
        std::string_view name, common::BufferView encoded_args) const override {
      return instance_->callExportFunction(name, encoded_args);
    }

    outcome::result<std::optional<WasmValue>> getGlobal(
        std::string_view name) const override {
      return instance_->getGlobal(name);
    }

    void forDataSegment(const DataSegmentProcessor &callback) const override {
      return instance_->forDataSegment(callback);
    }

    const InstanceEnvironment &getEnvironment() const override {
      return instance_->getEnvironment();
    }

    outcome::result<void> resetEnvironment() override {
      return instance_->resetEnvironment();
    }

   private:
    std::weak_ptr<RuntimeInstancesPool> pool_;
    RuntimeInstancesPool::RootHash state_;
    std::shared_ptr<ModuleInstance> instance_;
  };

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::tryAcquire(
      const RuntimeInstancesPool::RootHash &state) {
    std::scoped_lock guard{mt_};
    auto &pool = pools_[state];

    if (not pool.empty()) {
      auto top = std::move(pool.top());
      pool.pop();
      return std::make_shared<BorrowedInstance>(
          weak_from_this(), state, std::move(top));
    }

    auto opt_module = modules_.get(state);
    BOOST_ASSERT(opt_module.has_value());
    auto module = opt_module.value();
    OUTCOME_TRY(instance, module.get()->instantiate());

    return std::make_shared<BorrowedInstance>(
        weak_from_this(), state, std::move(instance));
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

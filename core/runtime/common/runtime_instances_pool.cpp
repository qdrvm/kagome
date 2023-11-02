/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_instances_pool.hpp"



#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"

namespace kagome::runtime {
  /**
   * @brief Wrapper type over sptr<ModuleInstance>. Allows to return instance
   * back to the ModuleInstancePool upon destruction of
   * BorrowedInstance.
   */
  class BorrowedInstance final : public ModuleInstance {
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

    std::shared_ptr<const Module> getModule() const override {
      return instance_->getModule();
    }

    outcome::result<common::Buffer> callExportFunction(
        RuntimeContext &ctx,
        std::string_view name,
        common::BufferView encoded_args) const override {
      return instance_->callExportFunction(ctx, name, encoded_args);
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

  RuntimeInstancesPool::RuntimeInstancesPool() : pools_{MODULES_CACHE_SIZE} {}

  RuntimeInstancesPool::RuntimeInstancesPool(
      std::shared_ptr<ModuleFactory> module_factory, size_t capacity)
      : module_factory_{std::move(module_factory)}, pools_{capacity} {}

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::instantiate(const RootHash &code_hash,
                                    common::BufferView code_zstd) {
    std::unique_lock lock{mt_};
    auto entry = pools_.get(code_hash);
    if (not entry) {
      lock.unlock();
      common::Buffer code;
      OUTCOME_TRY(uncompressCodeIfNeeded(code_zstd, code));
      OUTCOME_TRY(module, module_factory_->make(code));
      lock.lock();
      entry = pools_.get(code_hash);
      if (not entry) {
        entry = pools_.put(code_hash, {std::move(module), {}});
      }
    }
    OUTCOME_TRY(instance, entry->get().instantiate(lock));
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), code_hash, std::move(instance));
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::tryAcquire(
      const RuntimeInstancesPool::RootHash &state) {
    std::unique_lock lock{mt_};
    auto entry = pools_.get(state);
    BOOST_ASSERT(entry);
    OUTCOME_TRY(instance, entry->get().instantiate(lock));
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), state, std::move(instance));
  }

  void RuntimeInstancesPool::release(
      const RuntimeInstancesPool::RootHash &state,
      std::shared_ptr<ModuleInstance> &&instance) {
    std::lock_guard guard{mt_};
    auto entry = pools_.get(state);
    if (not entry) {
      entry = pools_.put(state, {instance->getModule(), {}});
    }
    entry->get().instances.emplace_back(std::move(instance));
  }

  std::optional<std::shared_ptr<const Module>> RuntimeInstancesPool::getModule(
      const RuntimeInstancesPool::RootHash &state) {
    std::lock_guard guard{mt_};
    if (auto entry = pools_.get(state)) {
      return entry->get().module;
    }
    return std::nullopt;
  }

  void RuntimeInstancesPool::putModule(
      const RuntimeInstancesPool::RootHash &state,
      std::shared_ptr<Module> module) {
    std::lock_guard guard{mt_};
    if (not pools_.get(state)) {
      pools_.put(state, {std::move(module), {}});
    }
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::Entry::instantiate(std::unique_lock<std::mutex> &lock) {
    if (instances.empty()) {
      auto copy = module;
      lock.unlock();
      return copy->instantiate();
    }
    auto instance = std::move(instances.back());
    instances.pop_back();
    return instance;
  }
}  // namespace kagome::runtime

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
  class BorrowedInstance : public ModuleInstance {
   public:
    BorrowedInstance(std::weak_ptr<RuntimeInstancesPool> pool,
                     const common::Hash256 &hash,
                     std::shared_ptr<ModuleInstance> instance)
        : pool_{std::move(pool)}, hash_{hash}, instance_{std::move(instance)} {}
    ~BorrowedInstance() {
      if (auto pool = pool_.lock()) {
        pool->release(hash_, std::move(instance_));
      }
    }

    const common::Hash256 &getCodeHash() const override {
      return instance_->getCodeHash();
    }

    std::shared_ptr<const Module> getModule() const override {
      return instance_->getModule();
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
    common::Hash256 hash_;  // either trie hash or code hash
    std::shared_ptr<ModuleInstance> instance_;
  };

  RuntimeInstancesPool::RuntimeInstancesPool(
      std::shared_ptr<ModuleFactory> module_factory, size_t capacity)
      : module_factory_{std::move(module_factory)}, pools_{capacity} {}

  outcome::result<std::shared_ptr<ModuleInstance>, CompilationError>
  RuntimeInstancesPool::instantiateFromCode(const CodeHash &code_hash,
                                            common::BufferView code_zstd) {
    std::unique_lock lock{pools_mtx_};
    auto entry = pools_.get(code_hash);
    bool found = entry.has_value();
    if (not found) {
      lock.unlock();

      OUTCOME_TRY(new_entry, tryCompileModule(code_hash, code_zstd));
      entry = new_entry;
      lock.lock();
    }
    auto instance = entry->get().instantiate(lock);
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), code_hash, std::move(instance));
  }

  outcome::result<std::reference_wrapper<RuntimeInstancesPool::InstancePool>,
                  CompilationError>
  RuntimeInstancesPool::tryCompileModule(const CodeHash &code_hash,
                                         common::BufferView code_zstd) {
    std::unique_lock queue_lock{wait_queue_mtx_};
    if (auto it = wait_queue_.find(code_hash); it != wait_queue_.end()) {
      it->second.waiting_threads_num++;
      queue_lock.unlock();

      // wait until compilation finishes
      std::shared_lock entry_lock{it->second.mutex};
      BOOST_ASSERT(it->second.compilation_done);

      // if this is the last waiting thread, delete the entry
      if (it->second.waiting_threads_num.fetch_sub(1) == 1) {
        entry_lock.unlock();
        queue_lock.lock();
        wait_queue_.erase(it);
      }
      std::shared_lock pools_lock{pools_mtx_};
      auto opt_pool = pools_.get(code_hash);
      BOOST_ASSERT(opt_pool);
      BOOST_ASSERT(opt_pool->get().module);
      return *opt_pool;

    } else {
      auto [new_it, emplaced] =
          wait_queue_.emplace(std::piecewise_construct,
                              std::forward_as_tuple(code_hash),
                              std::tuple<>{});
      BOOST_ASSERT(emplaced);
      std::unique_lock l{new_it->second.mutex};
      queue_lock.unlock();

      common::Buffer code;
      if (!uncompressCodeIfNeeded(code_zstd, code)) {
        return CompilationError{"Failed to uncompress code"};
      }
      OUTCOME_TRY(module, module_factory_->make(code));

      new_it->second.compilation_done = true;

      std::unique_lock pools_lock{pools_mtx_};
      return pools_.put(code_hash, {std::move(module), {}});
      // unlock and let waiting threads proceed
    }
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPool::instantiateFromState(
      const RuntimeInstancesPool::TrieHash &state) {
    std::unique_lock lock{pools_mtx_};
    auto entry = pools_.get(state);
    BOOST_ASSERT(entry);
    auto instance = entry->get().instantiate(lock);
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), state, std::move(instance));
  }

  void RuntimeInstancesPool::release(
      const RuntimeInstancesPool::TrieHash &state,
      std::shared_ptr<ModuleInstance> &&instance) {
    std::lock_guard guard{pools_mtx_};
    auto entry = pools_.get(state);
    if (not entry) {
      entry = pools_.put(state, {instance->getModule(), {}});
    }
    entry->get().instances.emplace_back(std::move(instance));
  }

  std::optional<std::shared_ptr<const Module>> RuntimeInstancesPool::getModule(
      const RuntimeInstancesPool::TrieHash &state) {
    std::lock_guard guard{pools_mtx_};
    if (auto entry = pools_.get(state)) {
      return entry->get().module;
    }
    return std::nullopt;
  }

  void RuntimeInstancesPool::putModule(
      const RuntimeInstancesPool::TrieHash &state,
      std::shared_ptr<Module> module) {
    std::lock_guard guard{pools_mtx_};
    if (not pools_.get(state)) {
      pools_.put(state, {std::move(module), {}});
    }
  }

  std::shared_ptr<ModuleInstance>
  RuntimeInstancesPool::InstancePool::instantiate(
      std::unique_lock<std::shared_mutex> &lock) {
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

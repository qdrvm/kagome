/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_instances_pool.hpp"

#include "common/monadic_utils.hpp"
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
    std::optional<std::reference_wrapper<InstancePool>> pool_opt{};
    {
      std::unique_lock lock{pools_mtx_};
      pool_opt = pools_.get(code_hash);
    }
    while (!pool_opt) {
      if (auto future = getFutureCompiledModule(code_hash)) {
        OUTCOME_TRY(future->get());
        std::unique_lock lock{pools_mtx_};
        pool_opt = pools_.get(code_hash);
        // may be nullopt if some other pool kicked the desired pool from the
        // cache before we grabbed it
      } else {
        std::promise<CompilationResult> promise;
        auto module_res =
            tryCompileModule(code_hash, code_zstd, promise.get_future());
        if (module_res) {
          std::unique_lock lock{pools_mtx_};
          pool_opt = std::ref(
              pools_.put(code_hash, InstancePool{module_res.value(), {}}));
          promise.set_value(module_res);
        } else {
          promise.set_value(module_res);
          return module_res.error();
        }
      }
    }
    BOOST_ASSERT(pool_opt);
    std::unique_lock lock{pools_mtx_};
    auto instance = pool_opt->get().instantiate(lock);
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), code_hash, std::move(instance));
  }

  std::optional<std::shared_future<RuntimeInstancesPool::CompilationResult>>
  RuntimeInstancesPool::getFutureCompiledModule(
      const CodeHash &code_hash) const {
    std::unique_lock l{compiling_modules_mtx_};
    auto iter = compiling_modules_.find(code_hash);
    if (iter == compiling_modules_.end()) {
      return std::nullopt;
    }
    auto future = iter->second;
    l.unlock();
    return future;
  }

  RuntimeInstancesPool::CompilationResult
  RuntimeInstancesPool::tryCompileModule(
      const CodeHash &code_hash,
      common::BufferView code_zstd,
      std::shared_future<CompilationResult> future) {
    std::unique_lock l{compiling_modules_mtx_};
    auto [iter, inserted] = compiling_modules_.insert({code_hash, future});
    BOOST_ASSERT(inserted);
    l.unlock();

    common::Buffer code;
    std::optional<CompilationResult> res;
    if (!uncompressCodeIfNeeded(code_zstd, code)) {
      res = CompilationError{"Failed to uncompress code"};
    } else {
      res = common::map_result(module_factory_->make(code), [](auto &&module) {
        return std::shared_ptr<const Module>(module);
      });
    }
    BOOST_ASSERT(res);

    l.lock();
    compiling_modules_.erase(iter);
    return *res;
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
    std::unique_lock guard{pools_mtx_};
    auto entry = pools_.get(state);
    if (not entry) {
      entry = pools_.put(state, {instance->getModule(), {}});
    }
    entry->get().instances.emplace_back(std::move(instance));
  }

  std::optional<std::shared_ptr<const Module>> RuntimeInstancesPool::getModule(
      const RuntimeInstancesPool::TrieHash &state) {
    std::unique_lock guard{pools_mtx_};
    if (auto entry = pools_.get(state)) {
      return entry->get().module;
    }
    return std::nullopt;
  }

  void RuntimeInstancesPool::putModule(
      const RuntimeInstancesPool::TrieHash &state,
      std::shared_ptr<Module> module) {
    std::unique_lock guard{pools_mtx_};
    if (not pools_.get(state)) {
      pools_.put(state, {std::move(module), {}});
    }
  }

  std::shared_ptr<ModuleInstance>
  RuntimeInstancesPool::InstancePool::instantiate(
      std::unique_lock<std::mutex> &lock) {
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

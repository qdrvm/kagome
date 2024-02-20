/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_instances_pool.hpp"

#include "common/monadic_utils.hpp"
#include "runtime/common/stack_limiter.hpp"
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
    BorrowedInstance(std::weak_ptr<RuntimeInstancesPoolImpl> pool,
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
    std::weak_ptr<RuntimeInstancesPoolImpl> pool_;
    common::Hash256 hash_;  // either trie hash or code hash
    std::shared_ptr<ModuleInstance> instance_;
  };

  RuntimeInstancesPoolImpl::RuntimeInstancesPoolImpl(
      std::shared_ptr<ModuleFactory> module_factory,
      size_t capacity)
      : module_factory_{std::move(module_factory)},
        pools_{capacity} {
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPoolImpl::instantiateFromCode(
      const CodeHash &code_hash,
      common::BufferView code_zstd,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock lock{pools_mtx_};
    auto pool_opt = pools_.get(code_hash);

    if (!pool_opt) {
      lock.unlock();
      OUTCOME_TRY(module, tryCompileModule(code_hash, code_zstd, config));
      lock.lock();
      pool_opt = pools_.get(code_hash);
      if (!pool_opt) {
        pool_opt = std::ref(pools_.put(code_hash, InstancePool{module, {}}));
      }
    }
    BOOST_ASSERT(pool_opt);
    OUTCOME_TRY(instance, pool_opt->get().instantiate(lock));
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), code_hash, std::move(instance));
  }

  RuntimeInstancesPoolImpl::CompilationResult
  RuntimeInstancesPoolImpl::tryCompileModule(
      const CodeHash &code_hash,
      common::BufferView code_zstd,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock l{compiling_modules_mtx_};
    if (auto iter = compiling_modules_.find(code_hash);
        iter != compiling_modules_.end()) {
      std::shared_future<CompilationResult> future = iter->second;
      l.unlock();
      return future.get();
    }
    std::promise<CompilationResult> promise;
    auto [iter, is_inserted] =
        compiling_modules_.insert({code_hash, promise.get_future()});
    BOOST_ASSERT(is_inserted);
    BOOST_ASSERT(iter != compiling_modules_.end());
    l.unlock();

    common::Buffer code;
    std::optional<CompilationResult> res;
    if (!uncompressCodeIfNeeded(code_zstd, code)) {
      res = CompilationError{"Failed to uncompress code"};
    } else {
      if (config.memory_limits.max_stack_values_num) {
        auto instr_res = instrumentWithStackLimiter(
            code, *config.memory_limits.max_stack_values_num);
        if (!instr_res) {
          res = CompilationError{fmt::format(
              "Failed to inject stack limiter: {}", instr_res.error().msg)};
        }
        code = std::move(instr_res.value());
      }
      res = common::map_result(module_factory_->make(code), [](auto &&module) {
        return std::shared_ptr<const Module>(module);
      });
    }
    BOOST_ASSERT(res);

    l.lock();
    compiling_modules_.erase(iter);
    promise.set_value(*res);
    return *res;
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPoolImpl::instantiateFromState(
      const RuntimeInstancesPoolImpl::TrieHash &state,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock lock{pools_mtx_};
    auto entry = pools_.get(state);
    BOOST_ASSERT(entry);
    OUTCOME_TRY(instance, entry->get().instantiate(lock));
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), state, std::move(instance));
  }

  void RuntimeInstancesPoolImpl::release(
      const RuntimeInstancesPoolImpl::TrieHash &state,
      std::shared_ptr<ModuleInstance> &&instance) {
    std::unique_lock guard{pools_mtx_};
    auto entry = pools_.get(state);
    if (not entry) {
      entry = pools_.put(state, {instance->getModule(), {}});
    }
    entry->get().instances.emplace_back(std::move(instance));
  }

  std::optional<std::shared_ptr<const Module>>
  RuntimeInstancesPoolImpl::getModule(
      const RuntimeInstancesPoolImpl::TrieHash &state) {
    std::unique_lock guard{pools_mtx_};
    if (auto entry = pools_.get(state)) {
      return entry->get().module;
    }
    return std::nullopt;
  }

  void RuntimeInstancesPoolImpl::putModule(
      const RuntimeInstancesPoolImpl::TrieHash &state,
      std::shared_ptr<Module> module) {
    std::unique_lock guard{pools_mtx_};
    if (not pools_.get(state)) {
      pools_.put(state, {std::move(module), {}});
    }
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPoolImpl::InstancePool::instantiate(
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

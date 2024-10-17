/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/common/runtime_instances_pool.hpp"

#include "application/app_configuration.hpp"
#include "common/monadic_utils.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/wabt/instrument.hpp"

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
                     RuntimeContext::ContextParams config,
                     std::shared_ptr<ModuleInstance> instance)
        : pool_{std::move(pool)},
          hash_{hash},
          config_{std::move(config)},
          instance_{std::move(instance)} {}
    BorrowedInstance(const BorrowedInstance &) = delete;
    BorrowedInstance(BorrowedInstance &&) = delete;
    BorrowedInstance &operator=(const BorrowedInstance &) = delete;
    BorrowedInstance &operator=(BorrowedInstance &&) = delete;
    ~BorrowedInstance() override {
      if (auto pool = pool_.lock()) {
        pool->release(hash_, config_, std::move(instance_));
      }
    }

    common::Hash256 getCodeHash() const override {
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

    outcome::result<void> stateless() override {
      return instance_->stateless();
    }

   private:
    std::weak_ptr<RuntimeInstancesPoolImpl> pool_;
    common::Hash256 hash_;
    RuntimeContext::ContextParams config_;
    std::shared_ptr<ModuleInstance> instance_;
  };

  RuntimeInstancesPoolImpl::RuntimeInstancesPoolImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<ModuleFactory> module_factory,
      std::shared_ptr<WasmInstrumenter> instrument,
      size_t capacity)
      : cache_dir_{app_config.runtimeCacheDirPath()},
        module_factory_{std::move(module_factory)},
        instrument_{std::move(instrument)},
        pools_{capacity} {
    BOOST_ASSERT(module_factory_);
  }

  outcome::result<std::shared_ptr<ModuleInstance>>
  RuntimeInstancesPoolImpl::instantiateFromCode(
      const CodeHash &code_hash,
      const GetCode &get_code,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock lock{pools_mtx_};
    OUTCOME_TRY(module, getPool(lock, code_hash, get_code, config));
    OUTCOME_TRY(instance, module.get().instantiate(lock));
    BOOST_ASSERT(shared_from_this());
    return std::make_shared<BorrowedInstance>(
        weak_from_this(), code_hash, config, std::move(instance));
  }

  std::filesystem::path RuntimeInstancesPoolImpl::getCachePath(
      const CodeHash &code_hash,
      const RuntimeContext::ContextParams &config) const {
    std::string name;
    auto to = std::back_inserter(name);
    if (auto type = module_factory_->compilerType()) {
      fmt::format_to(to, "{}_", *type);
    } else {
      name.append("wasm_");
    }
    fmt::format_to(to, "{}_s", code_hash.toHex());
    if (auto &stack = config.memory_limits.max_stack_values_num) {
      fmt::format_to(to, "{}", *stack);
    }
    if (auto v = boost::get<HeapAllocStrategyDynamic>(
            &config.memory_limits.heap_alloc_strategy)) {
      name.append("_d");
      if (auto &max = v->maximum_pages) {
        fmt::format_to(to, "{}", *max);
      }
    } else {
      fmt::format_to(to,
                     "_s{}",
                     boost::get<HeapAllocStrategyStatic>(
                         config.memory_limits.heap_alloc_strategy)
                         .extra_pages);
    }
    return cache_dir_ / name;
  }

  outcome::result<void> RuntimeInstancesPoolImpl::precompile(
      const CodeHash &code_hash,
      const GetCode &get_code,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock lock{pools_mtx_};
    OUTCOME_TRY(getPool(lock, code_hash, get_code, config));
    return outcome::success();
  }

  std::optional<std::shared_ptr<const Module>>
  RuntimeInstancesPoolImpl::getModule(
      const CodeHash &code_hash, const RuntimeContext::ContextParams &config) {
    std::unique_lock lock{pools_mtx_};
    Key key{code_hash, config};
    auto pool_opt = pools_.get(key);
    if (!pool_opt) {
      return std::nullopt;
    }
    return pool_opt->get().module;
  }

  outcome::result<
      std::reference_wrapper<RuntimeInstancesPoolImpl::InstancePool>>
  RuntimeInstancesPoolImpl::getPool(
      std::unique_lock<std::mutex> &lock,
      const CodeHash &code_hash,
      const GetCode &get_code,
      const RuntimeContext::ContextParams &config) {
    Key key{code_hash, config};
    auto pool_opt = pools_.get(key);

    if (!pool_opt) {
      lock.unlock();
      OUTCOME_TRY(module, tryCompileModule(code_hash, get_code, config));
      lock.lock();
      pool_opt = pools_.get(key);
      if (!pool_opt) {
        pool_opt = std::ref(pools_.put(key, InstancePool{.module = module}));
      }
    }
    BOOST_ASSERT(pool_opt);
    return pool_opt.value();
  }

  RuntimeInstancesPoolImpl::CompilationResult
  RuntimeInstancesPoolImpl::tryCompileModule(
      const CodeHash &code_hash,
      const GetCode &get_code,
      const RuntimeContext::ContextParams &config) {
    std::unique_lock l{compiling_modules_mtx_};
    Key key{code_hash, config};
    if (auto iter = compiling_modules_.find(key);
        iter != compiling_modules_.end()) {
      std::shared_future<CompilationResult> future = iter->second;
      l.unlock();
      return future.get();
    }
    std::promise<CompilationResult> promise;
    auto [iter, is_inserted] =
        compiling_modules_.insert({key, promise.get_future()});
    BOOST_ASSERT(is_inserted);
    BOOST_ASSERT(iter != compiling_modules_.end());
    l.unlock();
    auto path = getCachePath(code_hash, config);
    auto res = [&]() -> CompilationResult {
      std::error_code ec;
      if (not std::filesystem::exists(path, ec)) {
        if (ec) {
          return ec;
        }
        OUTCOME_TRY(code_zstd, get_code());
        OUTCOME_TRY(code, uncompressCodeIfNeeded(*code_zstd));
        BOOST_OUTCOME_TRY(code,
                          instrument_->instrument(code, config.memory_limits));
        OUTCOME_TRY(module_factory_->compile(path, code, config));
      }
      const kagome::parachain::PvfWorkerInputCodeParams code_params = {
          .path = path, .context_params = config};
      OUTCOME_TRY(module, module_factory_->loadCompiled(code_params));
      return module;
    }();
    l.lock();
    compiling_modules_.erase(iter);
    promise.set_value(res);
    return res;
  }

  void RuntimeInstancesPoolImpl::release(
      const CodeHash &code_hash,
      const RuntimeContext::ContextParams &config,
      std::shared_ptr<ModuleInstance> &&instance) {
    std::unique_lock guard{pools_mtx_};
    Key key{code_hash, config};
    auto entry = pools_.get(key);
    if (not entry) {
      entry = pools_.put(key, {.module = instance->getModule()});
    }
    entry->get().instances.emplace_back(std::move(instance));
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

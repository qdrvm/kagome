/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/core_api_factory_impl.hpp"

#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/runtime_properties_cache_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome::runtime::binaryen {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(
        const std::vector<uint8_t> &code,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory,
        const common::Hash256 &code_hash)
        : env_factory_{std::move(env_factory)},
          code_{code},
          code_hash_(code_hash) {
      BOOST_ASSERT(env_factory_);
    }

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        const primitives::BlockInfo &,
        const storage::trie::RootHash &) override {
      if (instance_ == nullptr) {
        auto module_res =
            ModuleImpl::createFromCode(code_, env_factory_, code_hash_);
        if (!module_res) return make_error_code(module_res.error());
        auto inst = module_res.value()->instantiate();
        instance_ = std::move(inst);
      }
      return instance_;
    }

   private:
    std::shared_ptr<ModuleInstance> instance_;
    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    const std::vector<uint8_t> &code_;
    const common::Hash256 code_hash_;
  };

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<runtime::RuntimePropertiesCache> cache)
      : instance_env_factory_{std::move(instance_env_factory)},
        header_repo_{std::move(header_repo)},
        cache_(std::move(cache)) {
    BOOST_ASSERT(instance_env_factory_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(cache_ != nullptr);
  }

  std::unique_ptr<Core> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    auto code_hash = hasher->sha2_256(runtime_code);
    auto ctx_factory = std::make_shared<RuntimeContextFactoryImpl>(
        std::make_shared<OneModuleRepository>(
            runtime_code, instance_env_factory_, code_hash),
        header_repo_);
    auto executor = std::make_unique<Executor>(ctx_factory, cache_);
    return std::make_unique<CoreImpl>(
        std::move(executor), ctx_factory, header_repo_, nullptr);
  }

}  // namespace kagome::runtime::binaryen

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/core_api_factory_impl.hpp"

#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/wasmedge/instance_environment_factory.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/module_impl.hpp"
#include "runtime/wasmedge/module_instance_impl.hpp"

namespace kagome::runtime::wasmedge {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(
        const std::vector<uint8_t> &code,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory)
        : env_factory_{std::move(env_factory)}, code_{code} {
      BOOST_ASSERT(env_factory_);
    }

    outcome::result<std::shared_ptr<runtime::ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider>,
        const primitives::BlockInfo &,
        const primitives::BlockHeader &) override {
      if (instance_ == nullptr) {
        OUTCOME_TRY(module, ModuleImpl::createFromCode(code_, env_factory_));
        OUTCOME_TRY(inst, module->instantiate());
        instance_ = std::move(inst);
      }
      return instance_;
    }

   private:
    std::shared_ptr<runtime::ModuleInstance> instance_;
    std::shared_ptr<const InstanceEnvironmentFactory> env_factory_;
    const std::vector<uint8_t> &code_;
  };

  class OneCodeProvider final : public RuntimeCodeProvider {
   public:
    explicit OneCodeProvider(gsl::span<const uint8_t> code) : code_{code} {}

    virtual outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &) const {
      return code_;
    }

   private:
    gsl::span<const uint8_t> code_;
  };

  CoreApiFactoryImpl::CoreApiFactoryImpl(
      std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : instance_env_factory_{std::move(instance_env_factory)},
        header_repo_{std::move(header_repo)},
        changes_tracker_{std::move(changes_tracker)} {
    BOOST_ASSERT(instance_env_factory_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
  }

  std::unique_ptr<Core> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    auto env_factory = std::make_shared<runtime::RuntimeEnvironmentFactory>(
        std::make_shared<OneCodeProvider>(runtime_code),
        std::make_shared<OneModuleRepository>(runtime_code,
                                              instance_env_factory_),
        header_repo_);
    auto executor = std::make_unique<Executor>(header_repo_, env_factory);
    return std::make_unique<CoreImpl>(
        std::move(executor), changes_tracker_, header_repo_);
  }

}  // namespace kagome::runtime::wasmedge

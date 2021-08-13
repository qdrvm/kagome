/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/executor_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome::runtime::binaryen {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(
        const std::vector<uint8_t> &code,
        std::shared_ptr<const InstanceEnvironmentFactory> env_factory)
        : env_factory_{std::move(env_factory)}, code_{code} {
      BOOST_ASSERT(env_factory_);
    }

    outcome::result<std::pair<std::shared_ptr<runtime::ModuleInstance>,
                              InstanceEnvironment>>
    getInstanceAt(std::shared_ptr<const RuntimeCodeProvider>,
                  const primitives::BlockInfo &b) override {
      if (instance_ == nullptr) {
        OUTCOME_TRY(module, ModuleImpl::createFromCode(code_, env_factory_));
        OUTCOME_TRY(inst_and_env, module->instantiate());
        instance_ = std::move(inst_and_env.first);
        env_ = std::move(inst_and_env.second);
      }
      return {instance_, env_};
    }

   private:
    std::shared_ptr<runtime::ModuleInstance> instance_;
    InstanceEnvironment env_;
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

  BinaryenExecutorFactory::BinaryenExecutorFactory(
      std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : instance_env_factory_{std::move(instance_env_factory)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(instance_env_factory_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  std::unique_ptr<Executor> BinaryenExecutorFactory::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    auto env_factory = std::make_shared<runtime::RuntimeEnvironmentFactory>(
        std::make_shared<OneCodeProvider>(runtime_code),
        std::make_shared<OneModuleRepository>(runtime_code, instance_env_factory_),
        header_repo_);
    auto executor = std::make_unique<Executor>(header_repo_, env_factory);
    return executor;
  }

}  // namespace kagome::runtime::binaryen

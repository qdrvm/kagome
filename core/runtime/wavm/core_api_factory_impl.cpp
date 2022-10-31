/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/core_api_factory_impl.hpp"

#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/executor.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_api/impl/core.hpp"
#include "runtime/runtime_api/impl/runtime_properties_cache_impl.hpp"
#include "runtime/runtime_environment_factory.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module.hpp"
#include "runtime/wavm/module_instance.hpp"
#include "runtime/wavm/module_params.hpp"

namespace kagome::runtime::wavm {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(
        std::shared_ptr<CompartmentWrapper> compartment,
        std::shared_ptr<ModuleParams> module_params,
        std::shared_ptr<IntrinsicModule> intrinsic_module,
        std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
        gsl::span<const uint8_t> code,
        std::shared_ptr<SingleModuleCache> last_compiled_module)
        : instance_env_factory_{std::move(instance_env_factory)},
          compartment_{compartment},
          module_params_{std::move(module_params)},
          intrinsic_module_{std::move(intrinsic_module)},
          code_{code},
          last_compiled_module_{last_compiled_module} {
      BOOST_ASSERT(instance_env_factory_);
      BOOST_ASSERT(compartment_);
      BOOST_ASSERT(intrinsic_module_);
      BOOST_ASSERT(last_compiled_module_);
    }

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider>,
        const primitives::BlockInfo &,
        const primitives::BlockHeader &) override {
      if (instance_ == nullptr) {
        auto module = ModuleImpl::compileFrom(compartment_,
                                              *module_params_,
                                              intrinsic_module_,
                                              instance_env_factory_,
                                              code_);
        OUTCOME_TRY(inst, module->instantiate());
        last_compiled_module_->set(std::move(module));
        instance_ = std::move(inst);
      }
      return instance_;
    }

   private:
    std::shared_ptr<ModuleInstance> instance_;
    std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory_;
    std::shared_ptr<CompartmentWrapper> compartment_;
    std::shared_ptr<ModuleParams> module_params_;
    std::shared_ptr<IntrinsicModule> intrinsic_module_;
    gsl::span<const uint8_t> code_;
    std::shared_ptr<SingleModuleCache> last_compiled_module_;
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
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<SingleModuleCache> last_compiled_module)
      : instance_env_factory_{std::move(instance_env_factory)},
        compartment_{compartment},
        module_params_{std::move(module_params)},
        intrinsic_module_{std::move(intrinsic_module)},
        storage_{std::move(storage)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)},
        last_compiled_module_{last_compiled_module} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(module_params_);
    BOOST_ASSERT(intrinsic_module_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(instance_env_factory_);
    BOOST_ASSERT(changes_tracker_);
    BOOST_ASSERT(last_compiled_module_);
  }

  std::unique_ptr<Core> CoreApiFactoryImpl::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    auto env_factory = std::make_shared<runtime::RuntimeEnvironmentFactory>(
        std::make_shared<OneCodeProvider>(runtime_code),
        std::make_shared<OneModuleRepository>(
            compartment_,
            module_params_,
            intrinsic_module_,
            instance_env_factory_,
            gsl::span<const uint8_t>{
                runtime_code.data(),
                static_cast<gsl::span<const uint8_t>::index_type>(
                    runtime_code.size())},
            last_compiled_module_),
        block_header_repo_);
    auto cache = std::make_shared<runtime::RuntimePropertiesCacheImpl>();
    auto executor = std::make_unique<runtime::Executor>(env_factory, cache);
    return std::make_unique<CoreImpl>(
        std::move(executor), changes_tracker_, block_header_repo_);
  }

}  // namespace kagome::runtime::wavm

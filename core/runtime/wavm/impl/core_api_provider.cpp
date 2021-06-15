/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/impl/core_api_provider.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/wavm/executor.hpp"
#include "runtime/wavm/impl/crutch.hpp"
#include "runtime/wavm/impl/intrinsic_module_instance.hpp"
#include "runtime/wavm/impl/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/impl/module_repository_impl.hpp"
#include "runtime/wavm/runtime_api/core.hpp"

namespace kagome::runtime::wavm {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(WAVM::Runtime::Compartment *compartment,
                        std::shared_ptr<IntrinsicResolver> resolver,
                        gsl::span<const uint8_t> code)
        : resolver_{std::move(resolver)},
          compartment_{compartment},
          code_{code} {
      BOOST_ASSERT(resolver_);
      BOOST_ASSERT(compartment_);
    }

    outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<RuntimeCodeProvider>,
        const primitives::BlockInfo &) override {
      if (instance_ == nullptr) {
        auto module = Module::compileFrom(compartment_, code_);
        instance_ = module->instantiate(*resolver_);
      }
      return instance_;
    }

    outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code) override {
      return Module::compileFrom(compartment_, byte_code);
    }

   private:
    std::shared_ptr<ModuleInstance> instance_;
    std::shared_ptr<IntrinsicResolver> resolver_;
    WAVM::Runtime::Compartment *compartment_;
    gsl::span<const uint8_t> code_;
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

  CoreApiProvider::CoreApiProvider(
      WAVM::Runtime::Compartment *compartment,
      std::shared_ptr<runtime::wavm::IntrinsicModuleInstance> intrinsic_module,
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory)
      : compartment_{compartment},
        intrinsic_module_{std::move(intrinsic_module)},
        storage_provider_{std::move(storage_provider)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)},
        host_api_factory_{std::move(host_api_factory)} {
    BOOST_ASSERT(compartment_);
    BOOST_ASSERT(intrinsic_module_);
    BOOST_ASSERT(storage_provider_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(changes_tracker_);
    BOOST_ASSERT(host_api_factory_);
  }

  std::unique_ptr<Core> CoreApiProvider::makeCoreApi(
      std::shared_ptr<const crypto::Hasher> hasher,
      gsl::span<uint8_t> runtime_code) const {
    auto new_intrinsic_module =
        std::shared_ptr<IntrinsicModuleInstance>(intrinsic_module_->clone(compartment_));
    auto new_memory_provider =
        std::make_shared<WavmMemoryProvider>(new_intrinsic_module);
    auto executor = std::make_shared<runtime::wavm::Executor>(
        storage_provider_,
        new_memory_provider,
        std::make_shared<OneModuleRepository>(
            compartment_,
            std::make_shared<IntrinsicResolverImpl>(new_intrinsic_module,
                                                    compartment_),
            runtime_code),
        block_header_repo_,
        std::make_shared<OneCodeProvider>(runtime_code));
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        shared_from_this(), new_memory_provider, storage_provider_));
    pushHostApi(host_api);
    executor->setHostApi(host_api);
    return std::make_unique<WavmCore>(
        executor, changes_tracker_, block_header_repo_);
  }

}  // namespace kagome::runtime::wavm

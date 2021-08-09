/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/core_api_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/module/module_impl.hpp"
#include "runtime/common/constant_code_provider.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/core.hpp"

namespace kagome::runtime::binaryen {

  class OneModuleRepository final : public ModuleRepository {
   public:
    OneModuleRepository(const std::vector<uint8_t> &code,
                        std::shared_ptr<RuntimeExternalInterface> rei,
                        std::shared_ptr<TrieStorageProvider> storage_provider)
        : rei_{std::move(rei)},
          storage_provider_{std::move(storage_provider)},
          code_{code} {
      BOOST_ASSERT(rei_);
      BOOST_ASSERT(storage_provider_);
    }

    outcome::result<std::shared_ptr<runtime::ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider>,
        const primitives::BlockInfo &) override {
      if (instance_ == nullptr) {
        OUTCOME_TRY(module,
                    ModuleImpl::createFromCode(code_, storage_provider_, rei_));
        OUTCOME_TRY(inst, module->instantiate());
        instance_ = std::move(inst);
      }
      return instance_;
    }

   private:
    std::shared_ptr<runtime::ModuleInstance> instance_;
    std::shared_ptr<RuntimeExternalInterface> rei_;
    std::shared_ptr<TrieStorageProvider> storage_provider_;
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

  BinaryenCoreApiFactory::BinaryenCoreApiFactory(
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory,
      std::shared_ptr<const host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage)
      : changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)},
        memory_factory_{std::move(memory_factory)},
        host_api_factory_{std::move(host_api_factory)},
        storage_{std::move(storage)} {
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(memory_factory_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
  }

  std::unique_ptr<Core> BinaryenCoreApiFactory::make(
      std::shared_ptr<const crypto::Hasher> hasher,
      const std::vector<uint8_t> &runtime_code) const {
    auto new_memory_provider =
        std::make_shared<BinaryenMemoryProvider>(memory_factory_);
    auto new_storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_);
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        shared_from_this(), new_memory_provider, new_storage_provider));
    auto external_interface =
        std::make_shared<RuntimeExternalInterface>(host_api);
    auto env_factory = std::make_shared<runtime::RuntimeEnvironmentFactory>(
        new_storage_provider,
        host_api,
        new_memory_provider,
        std::make_shared<OneCodeProvider>(runtime_code),
        std::make_shared<OneModuleRepository>(
            runtime_code, external_interface, new_storage_provider),
        header_repo_);
    auto executor =
        std::make_shared<Executor>(header_repo_, env_factory);
    return std::make_unique<CoreImpl>(executor, changes_tracker_, header_repo_);
  }

}  // namespace kagome::runtime::binaryen

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/instance_environment_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/wavm/core_api_factory_impl.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/module_params.hpp"
#include "runtime/wavm/wavm_external_memory_provider.hpp"
#include "runtime/wavm/wavm_internal_memory_provider.hpp"

namespace kagome::runtime::wavm {

  InstanceEnvironmentFactory::InstanceEnvironmentFactory(
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<ModuleParams> module_params,
      std::shared_ptr<IntrinsicModule> intrinsic_module,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<kagome::runtime::SingleModuleCache> last_compiled_module,
      std::shared_ptr<runtime::RuntimePropertiesCache> cache)
      : storage_{std::move(storage)},
        serializer_{std::move(serializer)},
        compartment_{std::move(compartment)},
        module_params_{std::move(module_params)},
        intrinsic_module_{std::move(intrinsic_module)},
        host_api_factory_{std::move(host_api_factory)},
        block_header_repo_{std::move(block_header_repo)},
        last_compiled_module_{std::move(last_compiled_module)},
        cache_(std::move(cache)) {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(module_params_ != nullptr);
    BOOST_ASSERT(intrinsic_module_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(block_header_repo_ != nullptr);
    BOOST_ASSERT(last_compiled_module_ != nullptr);
    BOOST_ASSERT(cache_ != nullptr);
  }

  InstanceEnvironment InstanceEnvironmentFactory::make(
      MemoryOrigin memory_origin,
      WAVM::Runtime::Instance *runtime_instance,
      std::shared_ptr<IntrinsicModuleInstance> intrinsic_instance) const {
    auto new_storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_, serializer_);
    auto core_factory =
        std::make_shared<CoreApiFactoryImpl>(compartment_,
                                             module_params_,
                                             intrinsic_module_,
                                             storage_,
                                             block_header_repo_,
                                             shared_from_this(),
                                             last_compiled_module_,
                                             cache_);

    std::shared_ptr<MemoryProvider> memory_provider;
    switch (memory_origin) {
      case MemoryOrigin::EXTERNAL:
        memory_provider =
            std::make_shared<WavmExternalMemoryProvider>(intrinsic_instance);
        break;
      case MemoryOrigin::INTERNAL: {
        auto *memory = WAVM::Runtime::getDefaultMemory(runtime_instance);
        memory_provider = std::make_shared<WavmInternalMemoryProvider>(memory);
      } break;
    }
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        core_factory, memory_provider, new_storage_provider));

    return InstanceEnvironment{std::move(memory_provider),
                               std::move(new_storage_provider),
                               std::move(host_api),
                               {}};
  }

}  // namespace kagome::runtime::wavm

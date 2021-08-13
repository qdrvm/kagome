/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wavm/instance_environment_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/wavm/executor_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module_instance.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/wavm_memory_provider.hpp"

namespace kagome::runtime::wavm {

  InstanceEnvironmentFactory::InstanceEnvironmentFactory(
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<CompartmentWrapper> compartment,
      std::shared_ptr<runtime::wavm::IntrinsicModule> intrinsic_module,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo)
      : storage_{std::move(storage)},
        compartment_{std::move(compartment)},
        intrinsic_module_{std::move(intrinsic_module)},
        host_api_factory_{std::move(host_api_factory)},
        block_header_repo_{std::move(block_header_repo)} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(compartment_ != nullptr);
    BOOST_ASSERT(intrinsic_module_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(block_header_repo_ != nullptr);
  }

  WavmInstanceEnvironment InstanceEnvironmentFactory::make() const {
    auto new_intrinsic_module_instance =
        std::shared_ptr<IntrinsicModuleInstance>(
            intrinsic_module_->instantiate());
    auto new_memory_provider = std::make_shared<WavmMemoryProvider>(
        new_intrinsic_module_instance, compartment_);
    auto new_storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_);
    auto executor_factory = std::make_shared<ExecutorFactory>(
        compartment_, storage_, block_header_repo_, shared_from_this());
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        executor_factory, new_memory_provider, new_storage_provider));
    pushHostApi(host_api);

    return WavmInstanceEnvironment{
        InstanceEnvironment{std::move(new_memory_provider),
                            std::move(new_storage_provider),
                            std::move(host_api)},
        std::make_shared<IntrinsicResolverImpl>(new_intrinsic_module_instance)};
  }

}  // namespace kagome::runtime::wavm

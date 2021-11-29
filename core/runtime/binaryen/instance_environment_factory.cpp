/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/binaryen/instance_environment_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/core_api_factory_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"

namespace kagome::runtime::binaryen {

  InstanceEnvironmentFactory::InstanceEnvironmentFactory(
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : storage_{std::move(storage)},
        serializer_{std::move(serializer)},
        host_api_factory_{std::move(host_api_factory)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)} {
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(changes_tracker_);
  }

  BinaryenInstanceEnvironment InstanceEnvironmentFactory::make() const {
    auto memory_factory = std::make_shared<BinaryenMemoryFactory>();
    auto new_memory_provider =
        std::make_shared<BinaryenMemoryProvider>(memory_factory);
    auto new_storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_, serializer_);
    auto core_factory = std::make_shared<CoreApiFactoryImpl>(
        shared_from_this(), block_header_repo_, changes_tracker_);
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        core_factory, new_memory_provider, new_storage_provider));
    auto rei = std::make_shared<RuntimeExternalInterface>(host_api);
    new_memory_provider->setExternalInterface(rei);
    return BinaryenInstanceEnvironment{
        InstanceEnvironment{std::move(new_memory_provider),
                            std::move(new_storage_provider),
                            std::move(host_api),
                            [](auto &) {}},
        std::move(rei)};
  }
}  // namespace kagome::runtime::binaryen

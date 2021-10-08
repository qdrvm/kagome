/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TEST_HPP
#define KAGOME_RUNTIME_TEST_HPP

#include "core/runtime/runtime_test_base.hpp"

#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/core_api_factory_impl.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"

class BinaryenRuntimeTest : public RuntimeTestBase {
 public:
  virtual ImplementationSpecificRuntimeClasses getImplementationSpecific() {
    auto memory_factory =
        std::make_shared<kagome::runtime::binaryen::BinaryenMemoryFactory>();
    auto memory_provider =
        std::make_shared<kagome::runtime::binaryen::BinaryenMemoryProvider>(
            memory_factory);
    auto trie_db = std::make_shared<kagome::storage::trie::TrieStorageMock>();

    auto instance_env_factory =
        std::make_shared<kagome::runtime::binaryen::InstanceEnvironmentFactory>(
            trie_db, host_api_factory_, header_repo_, changes_tracker_);

    auto core_api_factory =
        std::make_shared<kagome::runtime::binaryen::CoreApiFactoryImpl>(
            instance_env_factory, header_repo_, changes_tracker_);
    std::shared_ptr<kagome::host_api::HostApi> host_api =
        host_api_factory_->make(
            core_api_factory, memory_provider, storage_provider_);

    rei_ =
        std::make_shared<kagome::runtime::binaryen::RuntimeExternalInterface>(
            host_api);
    memory_provider->setExternalInterface(rei_);

    auto module_factory =
        std::make_shared<kagome::runtime::binaryen::ModuleFactoryImpl>(
            instance_env_factory, trie_db);
    return ImplementationSpecificRuntimeClasses{
        module_factory, core_api_factory, memory_provider, host_api};
  }

 private:
  // need to keep it alive as it's not owned by executor
  std::shared_ptr<kagome::runtime::binaryen::RuntimeExternalInterface> rei_;
};

#endif  // KAGOME_RUNTIME_TEST_HPP

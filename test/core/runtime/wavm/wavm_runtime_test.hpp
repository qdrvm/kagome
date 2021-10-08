/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WAVM_RUNTIME_TEST_HPP
#define KAGOME_WAVM_RUNTIME_TEST_HPP

#include "core/runtime/runtime_test_base.hpp"

#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/core_api_factory_impl.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "runtime/wavm/wavm_external_memory_provider.hpp"

class WavmRuntimeTest : public RuntimeTestBase {
 public:
  virtual ImplementationSpecificRuntimeClasses getImplementationSpecific() {
    auto compartment =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            "Test Compartment");
    auto intrinsic_module =
        std::make_shared<kagome::runtime::wavm::IntrinsicModule>(compartment);
    kagome::runtime::wavm::registerHostApiMethods(*intrinsic_module);
    std::shared_ptr<kagome::runtime::wavm::IntrinsicModuleInstance>
        intrinsic_module_instance = intrinsic_module->instantiate();
    resolver_ = std::make_shared<kagome::runtime::wavm::IntrinsicResolverImpl>(
        intrinsic_module_instance);
    auto trie_db = std::make_shared<kagome::storage::trie::TrieStorageMock>();
    auto changes_tracker =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();

    auto instance_env_factory = std::make_shared<kagome::runtime::wavm::InstanceEnvironmentFactory>(
        trie_db,
        compartment,
        intrinsic_module,
        host_api_factory_,
        header_repo_,
        changes_tracker);

    auto module_factory =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(compartment,
                                                                   instance_env_factory,
                                                                   intrinsic_module);

    auto memory_provider =
        std::make_shared<kagome::runtime::wavm::WavmExternalMemoryProvider>(
            intrinsic_module_instance);

    auto core_api_factory =
        std::make_shared<kagome::runtime::wavm::CoreApiFactoryImpl>(
            compartment,
            intrinsic_module,
            trie_db,
            header_repo_,
            instance_env_factory,
            changes_tracker_);

    std::shared_ptr<kagome::host_api::HostApi> host_api =
        host_api_factory_->make(
            core_api_factory, memory_provider, storage_provider_);

    kagome::runtime::wavm::pushHostApi(host_api);

    return ImplementationSpecificRuntimeClasses{
        module_factory, core_api_factory, memory_provider, host_api};
  }

 private:
  std::shared_ptr<kagome::runtime::wavm::IntrinsicResolver> resolver_;
};

#endif  // KAGOME_WAVM_RUNTIME_TEST_HPP

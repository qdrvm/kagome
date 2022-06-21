/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WAVM_RUNTIME_TEST_HPP
#define KAGOME_WAVM_RUNTIME_TEST_HPP

#include "core/runtime/runtime_test_base.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/module.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/core_api_factory_impl.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module_cache.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "runtime/wavm/module_params.hpp"
#include "runtime/wavm/wavm_external_memory_provider.hpp"

class WavmRuntimeTest : public RuntimeTestBase {
 public:
  virtual std::shared_ptr<kagome::runtime::ModuleFactory> createModuleFactory()
      override {
    auto compartment =
        std::make_shared<kagome::runtime::wavm::CompartmentWrapper>(
            "Test Compartment");
    auto module_params =
        std::make_shared<kagome::runtime::wavm::ModuleParams>();
    auto intrinsic_module =
        std::make_shared<kagome::runtime::wavm::IntrinsicModule>(
            compartment, module_params->intrinsicMemoryType);
    kagome::runtime::wavm::registerHostApiMethods(*intrinsic_module);
    std::shared_ptr<kagome::runtime::wavm::IntrinsicModuleInstance>
        intrinsic_module_instance = intrinsic_module->instantiate();
    resolver_ = std::make_shared<kagome::runtime::wavm::IntrinsicResolverImpl>(
        intrinsic_module_instance);
    auto changes_tracker =
        std::make_shared<kagome::storage::changes_trie::ChangesTrackerMock>();

    auto instance_env_factory =
        std::make_shared<kagome::runtime::wavm::InstanceEnvironmentFactory>(
            trie_storage_,
            serializer_,
            compartment,
            module_params,
            intrinsic_module,
            host_api_factory_,
            header_repo_,
            changes_tracker,
            std::make_shared<kagome::runtime::SingleModuleCache>());

    kagome::application::AppConfigurationMock config{};
    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();
    auto module_cache =
        std::make_shared<kagome::runtime::wavm::ModuleCache>(config, hasher);

    auto module_factory =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
            config,
            compartment,
            module_params,
            instance_env_factory,
            intrinsic_module,
            module_cache);

    return module_factory;
  }

 private:
  std::shared_ptr<kagome::runtime::wavm::IntrinsicResolver> resolver_;
};

#endif  // KAGOME_WAVM_RUNTIME_TEST_HPP

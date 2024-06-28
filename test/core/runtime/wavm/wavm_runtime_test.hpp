/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "core/runtime/runtime_test_base.hpp"

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/common/core_api_factory_impl.hpp"
#include "runtime/module.hpp"
#include "runtime/wavm/compartment_wrapper.hpp"
#include "runtime/wavm/instance_environment_factory.hpp"
#include "runtime/wavm/intrinsics/intrinsic_functions.hpp"
#include "runtime/wavm/intrinsics/intrinsic_module.hpp"
#include "runtime/wavm/intrinsics/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module_factory_impl.hpp"
#include "runtime/wavm/module_params.hpp"
#include "runtime/wavm/wavm_external_memory_provider.hpp"

class WavmRuntimeTest : public RuntimeTestBaseImpl {
 public:
  std::shared_ptr<kagome::runtime::ModuleFactory> createModuleFactory()
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
        compartment, intrinsic_module_instance);

    auto module_factory =
        std::make_shared<kagome::runtime::wavm::ModuleFactoryImpl>(
            compartment,
            module_params,
            host_api_factory_,
            nullptr,
            trie_storage_,
            serializer_,
            intrinsic_module,
            hasher_);

    auto instance_env_factory =
        std::make_shared<kagome::runtime::wavm::InstanceEnvironmentFactory>(
            trie_storage_, serializer_, host_api_factory_, nullptr);

    return module_factory;
  }

 private:
  std::shared_ptr<kagome::runtime::wavm::IntrinsicResolver> resolver_;
};

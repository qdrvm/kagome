/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "core/runtime/runtime_test_base.hpp"

#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "runtime/binaryen/binaryen_memory_factory.hpp"
#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/core_api_factory_impl.hpp"
#include "runtime/binaryen/instance_environment_factory.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"

class BinaryenRuntimeTest : public RuntimeTestBase {
 public:
  std::shared_ptr<kagome::runtime::ModuleFactory> createModuleFactory()
      override {
    auto memory_factory =
        std::make_shared<kagome::runtime::binaryen::BinaryenMemoryFactory>();
    auto memory_provider =
        std::make_shared<kagome::runtime::binaryen::BinaryenMemoryProvider>(
            memory_factory);

    auto instance_env_factory =
        std::make_shared<kagome::runtime::binaryen::InstanceEnvironmentFactory>(
            trie_storage_,
            serializer_,
            host_api_factory_,
            header_repo_,
            cache_);

    auto module_factory =
        std::make_shared<kagome::runtime::binaryen::ModuleFactoryImpl>(
            instance_env_factory, trie_storage_, hasher_);
    return module_factory;
  }
};

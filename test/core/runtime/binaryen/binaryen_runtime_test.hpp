/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_TEST_HPP
#define KAGOME_RUNTIME_TEST_HPP

#include "core/runtime/runtime_test_base.hpp"

#include "runtime/binaryen/binaryen_memory_provider.hpp"
#include "runtime/binaryen/binaryen_wasm_memory_factory.hpp"
#include "runtime/binaryen/core_api_factory.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/binaryen/wasm_memory_impl.hpp"

class BinaryenRuntimeTest : public RuntimeTestBase {
 public:
  virtual ImplementationSpecificRuntimeClasses getImplementationSpecific() {
    auto memory_factory = std::make_shared<
        kagome::runtime::binaryen::BinaryenWasmMemoryFactory>();
    auto memory_provider =
        std::make_shared<kagome::runtime::binaryen::BinaryenMemoryProvider>(
            memory_factory);

    auto core_api_provider =
        std::make_shared<kagome::runtime::binaryen::BinaryenCoreApiFactory>(
            changes_tracker_,
            header_repo_,
            memory_factory,
            host_api_factory_,
            std::make_shared<kagome::storage::trie::TrieStorageMock>());

    std::shared_ptr host_api = host_api_factory_->make(
        core_api_provider, memory_provider, storage_provider_);

    auto rei =
        std::make_shared<kagome::runtime::binaryen::RuntimeExternalInterface>(
            host_api);

    auto module_factory =
        std::make_shared<kagome::runtime::binaryen::ModuleFactoryImpl>(
            rei, storage_provider_);
    return ImplementationSpecificRuntimeClasses{
        module_factory, core_api_factory, memory_provider, host_api};
  }
};

#endif  // KAGOME_RUNTIME_TEST_HPP

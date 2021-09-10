/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/instance_environment_factory.hpp"

#include "host_api/host_api_factory.hpp"
#include "host_api/impl/host_api_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"
#include "runtime/wasmedge/core_api_factory_impl.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/register_host_api.hpp"

#include <wasmedge.h>

namespace kagome::runtime::wasmedge {

  InstanceEnvironmentFactory::InstanceEnvironmentFactory(
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<host_api::HostApiFactory> host_api_factory,
      std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : storage_{std::move(storage)},
        host_api_factory_{std::move(host_api_factory)},
        block_header_repo_{std::move(block_header_repo)},
        changes_tracker_{std::move(changes_tracker)} {
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(host_api_factory_);
    BOOST_ASSERT(block_header_repo_);
    BOOST_ASSERT(changes_tracker_);

    vm_ = WasmEdge_VMCreate(NULL, NULL);
    WasmEdge_String ExportName = WasmEdge_StringCreateByCString("ext");
    WasmEdge_ImportObjectContext *ImpObj =
        WasmEdge_ImportObjectCreate(ExportName, NULL);
    WasmEdge_StringDelete(ExportName);
    register_host_api(ImpObj);
    memory_provider_ = std::make_shared<WasmedgeMemoryProvider>();
    memory_provider_->setExternalInterface(ImpObj);
    WasmEdge_VMRegisterModuleFromImport(vm_, ImpObj);
  }

  WasmedgeInstanceEnvironment InstanceEnvironmentFactory::make() const {
    memory_provider_->resetMemory(0);
    auto new_storage_provider =
        std::make_shared<TrieStorageProviderImpl>(storage_);
    auto core_factory = std::make_shared<CoreApiFactoryImpl>(
        shared_from_this(), block_header_repo_, changes_tracker_);
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        core_factory, memory_provider_, new_storage_provider));
    return WasmedgeInstanceEnvironment{
        InstanceEnvironment{memory_provider_,
                            std::move(new_storage_provider),
                            std::move(host_api),
                            [](auto &) {}},
        vm_};
  }
}  // namespace kagome::runtime::wasmedge

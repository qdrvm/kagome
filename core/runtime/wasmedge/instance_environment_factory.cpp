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

namespace kagome::runtime::wasmedge {

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
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(host_api_factory_ != nullptr);
    BOOST_ASSERT(block_header_repo_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
  }

  WasmedgeInstanceEnvironment InstanceEnvironmentFactory::make() const {
    auto new_storage_provider =
      std::make_shared<TrieStorageProviderImpl>(storage_, serializer_);
    auto core_factory = std::make_shared<CoreApiFactoryImpl>(
        shared_from_this(), block_header_repo_, changes_tracker_);
    auto memory_provider = std::make_shared<WasmedgeMemoryProvider>();
    auto host_api = std::shared_ptr<host_api::HostApi>(host_api_factory_->make(
        core_factory, memory_provider, new_storage_provider));

    // WasmEdge_ImportObjectContext *ImpObj =
    //     WasmEdge_VMGetImportModuleContext(vm_, WasmEdge_HostRegistration_Wasi);
    // if (ImpObj) {
    //   register_host_api(ImpObj);
    //   memory_provider->setExternalInterface(ImpObj);
    // } else {
    //   WasmEdge_String ExportName = WasmEdge_StringCreateByCString("env");
    //   WasmEdge_ImportObjectContext *NewImpObj =
    //       WasmEdge_ImportObjectCreate(ExportName, host_api.get());
    //   WasmEdge_StringDelete(ExportName);
    //   register_host_api(NewImpObj);
    //   memory_provider->setExternalInterface(NewImpObj);
    //   WasmEdge_VMRegisterModuleFromImport(vm_, NewImpObj);
    // }

    return WasmedgeInstanceEnvironment{
        InstanceEnvironment{memory_provider,
                            std::move(new_storage_provider),
                            std::move(host_api),
                            [](auto &) {}}};
  }
}  // namespace kagome::runtime::wasmedge

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_CORE_API_FACTORY_IMPL_HPP
#define KAGOME_CORE_RUNTIME_WASMEDGE_CORE_API_FACTORY_IMPL_HPP

#include "runtime/core_api_factory_impl.hpp"
#include "runtime/runtime_api/core.hpp"

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class TrieStorageProvider;
  class Memory;
  class RuntimeEnvironmentFactory;
}  // namespace kagome::runtime

struct WasmEdge_ImportObjectContext;

namespace kagome::runtime::wasmedge {

  class InstanceEnvironmentFactory;
  class WasmedgeMemoryFactory;

  class CoreApiFactoryImpl final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<CoreApiFactoryImpl> {
   public:
    CoreApiFactoryImpl(
        WasmEdge_ImportObjectContext *ImpObj,
        std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker);

    std::unique_ptr<Core> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const override;

   private:
    std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    WasmEdge_ImportObjectContext *imp_obj_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_CORE_API_FACTORY_IMPL_HPP

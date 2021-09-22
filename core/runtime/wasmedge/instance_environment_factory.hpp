/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WASMEDGE_INSTANCE_ENVIRONMENT_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_WASMEDGE_INSTANCE_ENVIRONMENT_FACTORY_HPP

#include "runtime/instance_environment.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::storage::changes_trie {
  class ChangesTracker;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

struct WasmEdge_VMContext;

namespace kagome::runtime::wasmedge {

  class WasmedgeMemoryProvider;

  struct WasmedgeInstanceEnvironment {
    InstanceEnvironment env;
  };

  class InstanceEnvironmentFactory final
      : public std::enable_shared_from_this<InstanceEnvironmentFactory> {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker);

    [[nodiscard]] WasmedgeInstanceEnvironment make() const;

   private:
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<WasmedgeMemoryProvider> memory_provider_;
    WasmEdge_VMContext *vm_;
  };

}  // namespace kagome::runtime::wasmedge

#endif  // KAGOME_CORE_RUNTIME_WASMEDGE_INSTANCE_ENVIRONMENT_FACTORY_HPP

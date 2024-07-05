/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/instance_environment.hpp"

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace WAVM::Runtime {
  struct Instance;
}

namespace kagome::runtime {
  class CoreApiFactory;
}  // namespace kagome::runtime

namespace kagome::runtime::wavm {
  class IntrinsicModuleInstance;

  class InstanceEnvironmentFactory final
      : public std::enable_shared_from_this<InstanceEnvironmentFactory> {
   public:
    InstanceEnvironmentFactory(
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<CoreApiFactory> core_factory);

    enum class MemoryOrigin { EXTERNAL, INTERNAL };
    [[nodiscard]] InstanceEnvironment make(
        MemoryOrigin memory_origin,
        WAVM::Runtime::Instance *runtime_instance,
        std::shared_ptr<IntrinsicModuleInstance> intrinsic_instance) const;

   private:
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<CoreApiFactory> core_factory_;
  };

}  // namespace kagome::runtime::wavm

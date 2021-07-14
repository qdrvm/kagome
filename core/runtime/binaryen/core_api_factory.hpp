/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_HPP

#include "runtime/core_api_factory.hpp"
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

namespace kagome::runtime::binaryen {

  class BinaryenWasmMemoryFactory;

  class BinaryenCoreApiFactory final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<BinaryenCoreApiFactory> {
   public:
    BinaryenCoreApiFactory(
        std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory,
        std::shared_ptr<const host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage);

    // to avoid circular dependency
    void setRuntimeFactory(
        std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory) {
      runtime_env_factory_ = std::move(runtime_env_factory);
    }

    std::unique_ptr<Core> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t>& runtime_code) const override;

   private:
    std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory_;
    std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<const BinaryenWasmMemoryFactory> memory_factory_;
    std::shared_ptr<const host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_CORE_API_FACTORY_HPP

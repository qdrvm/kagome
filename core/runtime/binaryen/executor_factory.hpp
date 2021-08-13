/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BINARYEN_EXECUTOR_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_BINARYEN_EXECUTOR_FACTORY_HPP

#include "runtime/executor_factory.hpp"
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

  class InstanceEnvironmentFactory;
  class BinaryenMemoryFactory;

  class BinaryenExecutorFactory final
      : public runtime::ExecutorFactory,
        public std::enable_shared_from_this<BinaryenExecutorFactory> {
   public:
    BinaryenExecutorFactory(
        std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    // to avoid circular dependency
    void setRuntimeFactory(
        std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory) {
      runtime_env_factory_ = std::move(runtime_env_factory);
    }

    std::unique_ptr<Executor> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t>& runtime_code) const override;

   private:
    std::shared_ptr<const InstanceEnvironmentFactory> instance_env_factory_;
    std::shared_ptr<RuntimeEnvironmentFactory> runtime_env_factory_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::binaryen

#endif  // KAGOME_CORE_RUNTIME_BINARYEN_EXECUTOR_FACTORY_HPP

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/core_api_factory.hpp"
#include "runtime/wabt/instrument.hpp"

#include <memory>

#include "injector/lazy.hpp"

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class RuntimeInstancesPool;
}  // namespace kagome::runtime

namespace kagome::runtime {

  class CoreApiFactoryImpl final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<CoreApiFactoryImpl> {
   public:
    explicit CoreApiFactoryImpl(std::shared_ptr<crypto::Hasher> hasher,
                                LazySPtr<RuntimeInstancesPool> instance_pool);

    outcome::result<std::unique_ptr<RestrictedCore>> make(
        BufferView code,
        std::shared_ptr<TrieStorageProvider> storage_provider) const override;

   private:
    std::shared_ptr<crypto::Hasher> hasher_;
    LazySPtr<RuntimeInstancesPool> instance_pool_;
  };

}  // namespace kagome::runtime

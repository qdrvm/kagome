/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/core_api_factory.hpp"

#include <memory>

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime {
  class ModuleFactory;
  class SingleModuleCache;
}  // namespace kagome::runtime

namespace kagome::runtime {

  class CoreApiFactoryImpl final
      : public runtime::CoreApiFactory,
        public std::enable_shared_from_this<CoreApiFactoryImpl> {
   public:
    explicit CoreApiFactoryImpl(
        std::shared_ptr<const ModuleFactory> module_factory);
    ~CoreApiFactoryImpl() = default;

    outcome::result<std::unique_ptr<RestrictedCore>> make(
        std::shared_ptr<const crypto::Hasher> hasher,
        const std::vector<uint8_t> &runtime_code) const override;

   private:
    std::shared_ptr<const ModuleFactory> module_factory_;
  };

}  // namespace kagome::runtime

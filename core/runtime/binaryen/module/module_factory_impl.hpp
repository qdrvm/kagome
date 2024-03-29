/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::runtime::binaryen {

  class InstanceEnvironmentFactory;

  class ModuleFactoryImpl final
      : public ModuleFactory,
        public std::enable_shared_from_this<ModuleFactoryImpl> {
   public:
    ModuleFactoryImpl(std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                      std::shared_ptr<storage::trie::TrieStorage> storage,
                      std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<std::shared_ptr<Module>, CompilationError> make(
        common::BufferView code) const override;

   private:
    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime::binaryen

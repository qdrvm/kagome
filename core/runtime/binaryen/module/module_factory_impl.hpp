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

  class ModuleFactoryImpl final : public ModuleFactory {
   public:
    ModuleFactoryImpl(std::shared_ptr<InstanceEnvironmentFactory> env_factory,
                      std::shared_ptr<storage::trie::TrieStorage> storage,
                      std::shared_ptr<crypto::Hasher> hasher);

    // ModuleFactory
    std::optional<std::string> compilerType() const override;
    CompilationOutcome<void> compile(std::filesystem::path path_compiled,
                                     BufferView code) const override;
    CompilationOutcome<std::shared_ptr<Module>> loadCompiled(
        std::filesystem::path path_compiled) const override;

   private:
    std::shared_ptr<InstanceEnvironmentFactory> env_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::runtime::binaryen

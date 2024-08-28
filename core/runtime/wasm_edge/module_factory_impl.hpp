/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "log/logger.hpp"
#include "runtime/module_factory.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::runtime {
  class CoreApiFactory;
  class TrieStorageProvider;
}  // namespace kagome::runtime

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::runtime::wasm_edge {

  class ModuleFactoryImpl : public ModuleFactory {
   public:
    enum class ExecType {
      Interpreted,
      Compiled,
    };
    struct Config {
      Config(ExecType exec) : exec{exec} {}

      ExecType exec;
    };

    explicit ModuleFactoryImpl(
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<CoreApiFactory> core_factory,
        Config config);

    std::optional<std::string_view> compilerType() const override;
    CompilationOutcome<void> compile(std::filesystem::path path_compiled,
                                     BufferView code) const override;
    CompilationOutcome<std::shared_ptr<Module>> loadCompiled(
        std::filesystem::path path_compiled) const override;

   private:
    std::shared_ptr<const crypto::Hasher> hasher_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<CoreApiFactory> core_factory_;
    log::Logger log_;
    Config config_;
  };

}  // namespace kagome::runtime::wasm_edge

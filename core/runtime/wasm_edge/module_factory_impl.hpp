/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::host_api {
  class HostApiFactory;
}

namespace kagome::runtime {
  class TrieStorageProvider;
}

namespace kagome::blockchain {
  class BlockHeaderRepository;
}

namespace kagome::storage::trie {
  class TrieStorage;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::runtime::wasm_edge {

  class ModuleFactoryImpl : public ModuleFactory,
                            public std::enable_shared_from_this<ModuleFactoryImpl> {
   public:
    explicit ModuleFactoryImpl(
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<host_api::HostApiFactory> host_api_factory,
        std::shared_ptr<storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo);

    outcome::result<std::shared_ptr<Module>> make(
        gsl::span<const uint8_t> code) const override;

   private:
    std::shared_ptr<const crypto::Hasher> hasher_;
    std::shared_ptr<host_api::HostApiFactory> host_api_factory_;
    std::shared_ptr<storage::trie::TrieStorage> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
  };

}  // namespace kagome::runtime::wasm_edge

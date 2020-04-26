/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_IMPL_HPP
#define KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_IMPL_HPP

#include "storage/trie/readonly_trie_factory.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {

  class ReadonlyTrieFactoryImpl : public ReadonlyTrieFactory {
   public:
    explicit ReadonlyTrieFactoryImpl(
        std::shared_ptr<storage::trie::TrieDbBackend> backend);
    ~ReadonlyTrieFactoryImpl() override = default;

    std::unique_ptr<storage::trie::TrieDbReader> buildAt(
        primitives::BlockHash state_root) const override;

   private:
    std::shared_ptr<storage::trie::TrieDbBackend> backend_;
  };

}  // kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_READONLY_TRIE_FACTORY_IMPL_HPP

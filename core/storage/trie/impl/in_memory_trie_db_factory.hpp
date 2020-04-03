/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IN_MEMORY_TRIE_DB_FACTORY_HPP
#define KAGOME_IN_MEMORY_TRIE_DB_FACTORY_HPP

#include "storage/trie/trie_db_factory.hpp"

namespace kagome::storage::trie {

  class InMemoryTrieDbFactory: public TrieDbFactory {
   public:
    InMemoryTrieDbFactory() = default;
    ~InMemoryTrieDbFactory() override = default;

    std::unique_ptr<TrieDb> makeTrieDb() const override;

   private:

  };

}

#endif  // KAGOME_IN_MEMORY_TRIE_DB_FACTORY_HPP

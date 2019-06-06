/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP_
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP_

#include "common/buffer.hpp"
#include "storage/face/write_batch.hpp"
#include "storage/trie/polkadot_trie_db/polkadot_trie_db.hpp"

namespace kagome::storage::trie {
  class PolkadotTrieBatch
      : public face::WriteBatch<common::Buffer, common::Buffer> {
    enum class Action { PUT, REMOVE };
    struct Command {
      Action action;
      common::Buffer key;
      common::Buffer value; // empty for remove
    };

   public:
    PolkadotTrieBatch(PolkadotTrieDb &trie);
    ~PolkadotTrieBatch() = default;
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;
    outcome::result<void> remove(const common::Buffer &key) override;
    outcome::result<void> commit() override;
    void clear() override;

   private:
    outcome::result<PolkadotNode> applyPut(common::Buffer key, common::Buffer value);
    outcome::result<PolkadotNode> applyRemove(common::Buffer key);

    PolkadotTrieDb trie_;
    std::list<Command> commands_;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP_

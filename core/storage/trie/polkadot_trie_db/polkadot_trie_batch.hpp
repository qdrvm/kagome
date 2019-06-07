/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP

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
      // value is unnecessary when action is REMOVE
      common::Buffer value {};
    };

   public:
    explicit PolkadotTrieBatch(PolkadotTrieDb &trie);
    ~PolkadotTrieBatch() override = default;

    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;
    outcome::result<void> remove(const common::Buffer &key) override;
    outcome::result<void> commit() override;
    void clear() override;
    bool is_empty() const;

   private:
    outcome::result<PolkadotTrieDb::NodePtr> applyPut(
        const PolkadotTrieDb::NodePtr& root, const common::Buffer &key,
        common::Buffer value);
    outcome::result<PolkadotTrieDb::NodePtr> applyRemove(
        PolkadotTrieDb::NodePtr root, const common::Buffer &key);

    PolkadotTrieDb &trie_;
    std::list<Command> commands_;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_BATCH_HPP

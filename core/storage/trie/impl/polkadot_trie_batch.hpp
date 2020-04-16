/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_BATCH_HPP
#define KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_BATCH_HPP

#include "common/buffer.hpp"
#include "storage/face/write_batch.hpp"
#include "storage/trie/impl/polkadot_trie_db.hpp"

namespace kagome::storage::trie {
  class PolkadotTrieBatch
      : public face::WriteBatch<common::Buffer, common::Buffer> {
    enum class Action { PUT, REMOVE };
    struct Command {
      Action action;
      common::Buffer key{};
      // value is unnecessary when action is REMOVE
      common::Buffer value{};
    };

   public:
    explicit PolkadotTrieBatch(PolkadotTrieDb &trie);
    ~PolkadotTrieBatch() override = default;

    // value will be copied
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;

    outcome::result<void> commit() override;

    void clear() override;

    bool is_empty() const;

   private:
    outcome::result<PolkadotTrieDb::NodePtr> applyPut(PolkadotTrie &trie, const common::Buffer &key,
        common::Buffer &&value);

    outcome::result<PolkadotTrieDb::NodePtr> applyRemove(PolkadotTrie &trie, const common::Buffer &key);

    PolkadotTrieDb &storage_;
    std::list<Command> commands_;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_IMPL_POLKADOT_TRIE_BATCH_HPP

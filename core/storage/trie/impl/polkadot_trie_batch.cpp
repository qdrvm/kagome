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

#include "storage/trie/impl/polkadot_trie_batch.hpp"

namespace kagome::storage::trie {

  using common::Buffer;

  PolkadotTrieBatch::PolkadotTrieBatch(PolkadotTrieDb &trie)
      : storage_{trie} {}

  outcome::result<void> PolkadotTrieBatch::put(const Buffer &key,
                                               const Buffer &value) {
    Buffer copy{value};
    return put(key, std::move(copy));
  }

  outcome::result<void> PolkadotTrieBatch::put(const Buffer &key,
                                               Buffer &&value) {
    if (value.empty()) {
      OUTCOME_TRY(remove(key));
    } else {
      commands_.push_back({Action::PUT, key, value});
    }
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieBatch::remove(const Buffer &key) {
    commands_.push_back({Action::REMOVE, key});
    return outcome::success();
  }

  outcome::result<void> PolkadotTrieBatch::commit() {
    if (commands_.empty()) {
      return outcome::success();
    }

    // make the command list empty and store commands in a local list
    // this is done because if an error occurs, commands that are not processed
    // yet must not remain in the batch
    decltype(commands_) commands{};
    std::swap(commands_, commands);

    OUTCOME_TRY(trie, storage_.initTrie());

    for (auto &command : commands) {
      switch (command.action) {
        case Action::PUT: {
          OUTCOME_TRY(applyPut(trie, command.key, std::move(command.value)));
          break;
        }
        case Action::REMOVE: {
          OUTCOME_TRY(applyRemove(trie, command.key));
          break;
        }
      }
    }
    if (trie.getRoot() == nullptr) {
      storage_.merkle_hash_ = storage_.getEmptyRoot();
    } else {
      OUTCOME_TRY(storage_.storeRootNode(*trie.getRoot()));
    }
    return outcome::success();
  }

  void PolkadotTrieBatch::clear() {
    commands_.clear();
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieBatch::applyPut(
      PolkadotTrie &trie, const common::Buffer &key, common::Buffer &&value) {
    OUTCOME_TRY(trie.put(key, value));
    return trie.getRoot();
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieBatch::applyRemove(
      PolkadotTrie &trie, const common::Buffer &key) {
    OUTCOME_TRY(trie.remove(key));
    return trie.getRoot();
  }

  bool PolkadotTrieBatch::is_empty() const {
    return commands_.empty();
  }

}  // namespace kagome::storage::trie

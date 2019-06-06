/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie_db/polkadot_trie_batch.hpp"

namespace kagome::storage::trie {

  using common::Buffer;

  PolkadotTrieBatch::PolkadotTrieBatch(PolkadotTrieDb &trie) : trie_{trie} {}

  outcome::result<void> PolkadotTrieBatch::put(const Buffer &key,
                                               const Buffer &value) {
    commands_.push_back({Action::PUT, key, value});
  }

  outcome::result<void> PolkadotTrieBatch::remove(const Buffer &key) {
    commands_.push_back({Action::REMOVE, key, Buffer{}});
  }

  outcome::result<void> PolkadotTrieBatch::commit() {
    PolkadotTrieDb::NodePtr new_root{};
    for (auto &command : commands_) {
      switch (command.action) {
        case Action::PUT:
          OUTCOME_TRY(
              n, applyPut(std::move(command.key), std::move(command.value)));
          new_root = n;
          break;
        case Action::REMOVE:
          OUTCOME_TRY(n, applyRemove(std::move(command.key)));
          new_root = n;
          break;
      }
    }
    trie_->storeNode(new_root);
    clear();
  }

  void PolkadotTrieBatch::clear() {
    commands_.clear();
  }

  outcome::result<PolkadotNode> PolkadotTrieBatch::applyPut(
      common::Buffer key, common::Buffer value) {
    auto k_enc = PolkadotCodec::keyToNibbles(key);

    if (value.empty()) {
      OUTCOME_TRY(remove(key));
    } else if (not trie_.root_.has_value()) {
      // will create a leaf node with provided key and value, save it to the
      // storage and return the key to it
      OUTCOME_TRY(trie_.insertRoot(k_enc, value));
    } else {
      OUTCOME_TRY(root, trie_.retrieveNode(trie_.root_.value()));
      // insert will pull a sequence of nodes (a path) from the storage and work
      // on it in memory
      OUTCOME_TRY(
          n, insert(root, k_enc, std::make_shared<LeafNode>(k_enc, value)));
      return n;
    }
    return outcome::success();
  }

  outcome::result<PolkadotNode> PolkadotTrieBatch::applyRemove(
      common::Buffer key) {

  }

}  // namespace kagome::storage::trie

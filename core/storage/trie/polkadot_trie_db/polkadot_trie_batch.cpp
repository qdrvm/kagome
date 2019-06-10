/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie_db/polkadot_trie_batch.hpp"

namespace kagome::storage::trie {

  using common::Buffer;

  PolkadotTrieBatch::PolkadotTrieBatch(PolkadotTrieDb &trie)
      : trie_{trie}, commands_ {} {}

  outcome::result<void> PolkadotTrieBatch::put(const Buffer &key,
                                               const Buffer &value) {
    Buffer copy {value};
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

    PolkadotTrieDb::NodePtr new_root = nullptr;
    if (not trie_.getRootHash().empty()) {
      OUTCOME_TRY(n, trie_.retrieveNode(trie_.getRootHash()));
      new_root = n;
    }
    for (auto &command : commands) {
      switch (command.action) {
        case Action::PUT: {
          OUTCOME_TRY(
              n, applyPut(new_root, command.key, std::move(command.value)));
          new_root = n;
          break;
        }
        case Action::REMOVE: {
          OUTCOME_TRY(n, applyRemove(new_root, command.key));
          new_root = n;
          break;
        }
      }
    }
    if (new_root == nullptr) {
      trie_.root_ = std::nullopt;
    } else {
      OUTCOME_TRY(n, trie_.storeNode(*new_root));
      trie_.root_ = n;
    }
    return outcome::success();
  }

  void PolkadotTrieBatch::clear() {
    commands_.clear();
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieBatch::applyPut(
      const PolkadotTrieDb::NodePtr &root, const common::Buffer &key,
      common::Buffer &&value) {
    auto k_enc = PolkadotCodec::keyToNibbles(key);

    // insert fetches a sequence of nodes (a path) from the storage and
    // these nodes are processed in memory, so any changes applied to them
    // will be written back to the storage only on storeNode call
    OUTCOME_TRY(
        n,
        trie_.insert(root, k_enc,
                     std::make_shared<LeafNode>(k_enc, std::move(value))));
    return std::move(n);
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieBatch::applyRemove(
      PolkadotTrieDb::NodePtr root, const common::Buffer &key) {
    auto key_nibbles = PolkadotCodec::keyToNibbles(key);
    // delete node will fetch nodes that it needs from the storage (the nodes
    // typically are a path in the trie) and work on them in memory
    OUTCOME_TRY(n, trie_.deleteNode(std::move(root), key_nibbles));
    return std::move(n);
  }

  bool PolkadotTrieBatch::is_empty() const {
    return commands_.empty();
  }

}  // namespace kagome::storage::trie

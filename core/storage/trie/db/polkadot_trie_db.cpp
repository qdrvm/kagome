/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/polkadot_trie_db.hpp"

#include <utility>

#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_node.hpp"
#include "storage/trie/impl/polkadot_trie.hpp"
#include "storage/trie/impl/polkadot_trie_batch.hpp"
#include "storage/trie/impl/trie_error.hpp"

using kagome::common::Buffer;

namespace kagome::storage::trie {

  std::unique_ptr<PolkadotTrieDb> PolkadotTrieDb::createFromStorage(
      common::Buffer root, std::shared_ptr<TrieDbBackend> backend) {
    BOOST_ASSERT(backend != nullptr);
    PolkadotTrieDb trie_db{std::move(backend), std::move(root)};
    return std::make_unique<PolkadotTrieDb>(std::move(trie_db));
  }

  std::unique_ptr<PolkadotTrieDb> PolkadotTrieDb::createEmpty(
      std::shared_ptr<TrieDbBackend> backend) {
    BOOST_ASSERT(backend != nullptr);
    PolkadotTrieDb trie_db{std::move(backend), boost::none};
    return std::make_unique<PolkadotTrieDb>(std::move(trie_db));
  }

  std::unique_ptr<TrieDbReader> PolkadotTrieDb::initReadOnlyFromStorage(
      common::Buffer root, std::shared_ptr<TrieDbBackend> backend) {
    return PolkadotTrieDb::createFromStorage(std::move(root),
                                             std::move(backend));
  }

  PolkadotTrieDb::PolkadotTrieDb(std::shared_ptr<TrieDbBackend> db,
                                 boost::optional<common::Buffer> root_hash)
      : db_{std::move(db)},
        merkle_hash_{root_hash ? std::move(root_hash.value())
                                     : PolkadotTrieDb::getEmptyRoot()} {}

  outcome::result<void> PolkadotTrieDb::put(const Buffer &key,
                                            const Buffer &value) {
    auto value_copy = value;
    return put(key, std::move(value_copy));
  }

  outcome::result<void> PolkadotTrieDb::put(const Buffer &key, Buffer &&value) {
    NodePtr root = nullptr;
    OUTCOME_TRY(trie, initTrie());
    // operations on the trie are done in memory
    OUTCOME_TRY(trie.put(key, value));
    // after this storeNode will recursively write all changed nodes back to
    // the storage and return the hash of the root node, which is used as a
    // key in the storage
    OUTCOME_TRY(storeRootNode(*trie.getRoot()));
    return outcome::success();
  }

  common::Buffer PolkadotTrieDb::getRootHash() {
    return merkle_hash_;
  }

  outcome::result<void> PolkadotTrieDb::clearPrefix(
      const common::Buffer &prefix) {
    if (empty()) {
      return outcome::success();
    }
    OUTCOME_TRY(trie, initTrie());
    OUTCOME_TRY(trie.clearPrefix(prefix));
    if (trie.getRoot() == nullptr) {
      merkle_hash_ = getEmptyRoot();
    } else {
      OUTCOME_TRY(storeRootNode(*trie.getRoot()));
    }
    return outcome::success();
  }

  std::unique_ptr<PolkadotTrieDb::WriteBatch> PolkadotTrieDb::batch() {
    // create a new batch and pass a reference to *this to it
    return std::make_unique<PolkadotTrieBatch>(*this);
  }

  std::unique_ptr<PolkadotTrieDb::MapCursor> PolkadotTrieDb::cursor() {
    return db_->cursor();  // perhaps should iterate over nodes in the trie
  }

  outcome::result<common::Buffer> PolkadotTrieDb::get(
      const common::Buffer &key) const {
    if (empty()) {
      return TrieError::NO_VALUE;
    }
    OUTCOME_TRY(trie, initTrie());
    return trie.get(key);
  }

  bool PolkadotTrieDb::contains(const common::Buffer &key) const {
    auto res = get(key);
    return res.has_value();
  }

  outcome::result<void> PolkadotTrieDb::remove(const common::Buffer &key) {
    if (empty()) {
      return outcome::success();
    }
    OUTCOME_TRY(trie, initTrie());
    // operations on the trie are done in memory
    OUTCOME_TRY(trie.remove(key));
    // after this, the nodes are written back to the storage and the new trie
    // root hash is obtained
    if (trie.getRoot() == nullptr) {
      merkle_hash_ = getEmptyRoot();
    } else {
      OUTCOME_TRY(storeRootNode(*trie.getRoot()));
    }
    return outcome::success();
  }

  outcome::result<PolkadotTrie> PolkadotTrieDb::initTrie() const {
    OUTCOME_TRY(root, retrieveNode(merkle_hash_));
    return PolkadotTrie{std::move(root),
                        [this](const BranchPtr &parent, uint8_t idx) {
                          return retrieveChild(parent, idx);
                        }};
  }

  common::Buffer PolkadotTrieDb::getEmptyRoot() const {
    return Buffer(codec_.hash256({0}));
  }

  bool PolkadotTrieDb::empty() const {
    return merkle_hash_ == getEmptyRoot();
  }

}  // namespace kagome::storage::trie

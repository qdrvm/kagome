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

  outcome::result<std::unique_ptr<PolkadotTrieDb>>
  PolkadotTrieDb::createFromStorage(std::shared_ptr<PolkadotTrieDbBackend> db) {
    BOOST_ASSERT(db != nullptr);
    OUTCOME_TRY(root, db->getRootHash());
    PolkadotTrieDb trie_db{db, std::move(root)};
    return std::make_unique<PolkadotTrieDb>(std::move(trie_db));
  }

  std::unique_ptr<PolkadotTrieDb> PolkadotTrieDb::createEmpty(
      std::shared_ptr<PolkadotTrieDbBackend> db) {
    BOOST_ASSERT(db != nullptr);
    PolkadotTrieDb trie_db{db, boost::none};
    return std::make_unique<PolkadotTrieDb>(std::move(trie_db));
  }

  PolkadotTrieDb::PolkadotTrieDb(std::shared_ptr<PolkadotTrieDbBackend> db,
                                 boost::optional<common::Buffer> root_hash)
      : db_{std::move(db)},
        codec_{},
        root_{root_hash ? std::move(root_hash.value()) : getEmptyRoot()} {
  }

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
    OUTCOME_TRY(root_hash, storeNode(*trie.getRoot()));
    root_ = root_hash;
    OUTCOME_TRY(db_->saveRootHash(root_));
    return outcome::success();
  }

  common::Buffer PolkadotTrieDb::getRootHash() const {
    // if the length of the encoded root is less than 32, it is not hashed,
    // so hash it in this case
    return root_.size() < 32 ? Buffer{codec_.hash256(root_)} : root_;
  }

  outcome::result<void> PolkadotTrieDb::clearPrefix(
      const common::Buffer &prefix) {
    if (empty()) {
      return outcome::success();
    }
    OUTCOME_TRY(trie, initTrie());
    OUTCOME_TRY(trie.clearPrefix(prefix));
    if (trie.getRoot() == nullptr) {
      root_ = getEmptyRoot();
    } else {
      OUTCOME_TRY(hash, storeNode(*trie.getRoot()));
      root_ = hash;
    }
    OUTCOME_TRY(db_->saveRootHash(root_));
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
      root_ = getEmptyRoot();
    } else {
      OUTCOME_TRY(root_hash, storeNode(*trie.getRoot()));
      root_ = root_hash;
    }
    OUTCOME_TRY(db_->saveRootHash(root_));
    return outcome::success();
  }

  outcome::result<PolkadotTrie> PolkadotTrieDb::initTrie() const {
    OUTCOME_TRY(root, retrieveNode(root_));
    return PolkadotTrie{std::move(root),
                        [this](const BranchPtr &parent, uint8_t idx) {
                          return retrieveChild(parent, idx);
                        }};
  }

  outcome::result<common::Buffer> PolkadotTrieDb::storeNode(
      PolkadotNode &node) {
    auto batch = db_->batch();
    OUTCOME_TRY(hash, storeNode(node, *batch));
    OUTCOME_TRY(batch->commit());
    return std::move(hash);
  }

  outcome::result<common::Buffer> PolkadotTrieDb::storeNode(PolkadotNode &node,
                                                            WriteBatch &batch) {
    using T = PolkadotNode::Type;

    // if node is a branch node, its children must be stored to the storage
    // before it, as their hashes, which are used as database keys, are a part
    // of its encoded representation required to save it to the storage
    if (node.getTrieType() == T::BranchEmptyValue
        || node.getTrieType() == T::BranchWithValue) {
      auto &branch = dynamic_cast<BranchNode &>(node);
      for (auto &child : branch.children) {
        if (child and not child->isDummy()) {
          OUTCOME_TRY(hash, storeNode(*child));
          // when a node is written to the storage, it is replaced with a dummy
          // node to avoid memory waste
          child = std::make_shared<DummyNode>(hash);
        }
      }
    }

    OUTCOME_TRY(enc, codec_.encodeNode(node));
    auto key = Buffer{codec_.hash256(enc)};
    OUTCOME_TRY(batch.put(key, enc));
    return key;
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::retrieveChild(
      const BranchPtr &parent, uint8_t idx) const {
    if (parent->children.at(idx) == nullptr) {
      return nullptr;
    }
    if (parent->children.at(idx)->isDummy()) {
      auto dummy =
          std::dynamic_pointer_cast<DummyNode>(parent->children.at(idx));
      OUTCOME_TRY(n, retrieveNode(dummy->db_key));
      parent->children.at(idx) = n;
    }
    return parent->children.at(idx);
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::retrieveNode(
      const common::Buffer &db_key) const {
    if (db_key.empty() or db_key == getEmptyRoot()) {
      return nullptr;
    }
    OUTCOME_TRY(enc, db_->get(db_key));
    OUTCOME_TRY(n, codec_.decodeNode(enc));
    return std::dynamic_pointer_cast<PolkadotNode>(n);
  }

  common::Buffer PolkadotTrieDb::getEmptyRoot() const {
    return Buffer{}.put(codec_.hash256({0}));
  }

  bool PolkadotTrieDb::empty() const {
    return root_ == getEmptyRoot();
  }

}  // namespace kagome::storage::trie

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/merkle/polkadot_trie_db/polkadot_trie_db.hpp"

#include <utility>

#include "scale/byte_array_stream.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_codec.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"

using kagome::common::Buffer;

namespace {
  /**
   * Returns a subspan of a buffer
   * See gsl::span
   */
  inline auto subbuffer(const Buffer &key, size_t offset = 0,
                        size_t length = -1) {
    return Buffer(
        gsl::span<const uint8_t>(key.toVector()).subspan(offset, length));
  }
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::merkle, PolkadotTrieDb::Error, e) {
  using E = kagome::storage::merkle::PolkadotTrieDb::Error;
  switch (e) {
    case E::INVALID_NODE_TYPE:
      return "The node type is invalid";
  }
  return "Unknown error";
}

namespace kagome::storage::merkle {

  PolkadotTrieDb::PolkadotTrieDb(std::unique_ptr<PersistentBufferMap> db,
                                 std::shared_ptr<Codec> codec,
                                 std::shared_ptr<hash::Hasher> hasher)
      : db_{std::move(db)},
        hasher_{std::move(hasher)},
        codec_{std::move(codec)} {}

  outcome::result<void> PolkadotTrieDb::put(const Buffer &key,
                                            const Buffer &value) {
    auto k_enc = PolkadotCodec::keyToNibbles(key);

    if (value.empty()) {
      OUTCOME_TRY(remove(key));
    } else if (not root_.has_value()) {
      // will create a leaf node with provided key and value, save it to the
      // storage and return the key to it
      OUTCOME_TRY(insertRoot(k_enc, value));
    } else {
      OUTCOME_TRY(root, retrieveNode(root_.value()));
      // insert will pull a sequence of nodes (a path) from the storage and work
      // on it in memory
      OUTCOME_TRY(
          n, insert(root, k_enc, std::make_shared<LeafNode>(k_enc, value)));
      // after this storeNode will recursively write all changed nodes back to
      // the storage and return the hash of the root node, which is used as a
      // key in the storage
      OUTCOME_TRY(root_hash, storeNode(*n));
      root_ = root_hash;
    }
    return outcome::success();
  }

  common::Buffer PolkadotTrieDb::getRootHash() const {
    return root_.value_or(Buffer{});
  }

  outcome::result<void> PolkadotTrieDb::clearPrefix(
      const common::Buffer &prefix) {
    if (not root_.has_value()) {
      return outcome::success();
    }
    auto key_nibbles = PolkadotCodec::keyToNibbles(prefix);
    OUTCOME_TRY(root, retrieveNode(root_.value()));
    OUTCOME_TRY(new_root, detachNode(root, key_nibbles));
    if (new_root == nullptr) {
      root_ = std::nullopt;
    } else {
      OUTCOME_TRY(hash, storeNode(*new_root));
      root_ = hash;
    }
    return outcome::success();
  }

  std::unique_ptr<PolkadotTrieDb::WriteBatch> PolkadotTrieDb::batch() {
    // TODO(Harrm) PRE-199 Implement batch in merkle trie
    return nullptr;
  }

  std::unique_ptr<PolkadotTrieDb::MapCursor> PolkadotTrieDb::cursor() {
    return db_->cursor();  // perhaps should iterate over nodes in the trie
  }

  outcome::result<void> PolkadotTrieDb::insertRoot(
      const common::Buffer &key_nibbles, const common::Buffer &value) {
    LeafNode root(key_nibbles, value);
    OUTCOME_TRY(root_hash, storeNode(root));
    root_ = root_hash;
    return outcome::success();
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::insert(
      const NodePtr &parent, const common::Buffer &key_nibbles, NodePtr node) {
    using T = PolkadotNode::Type;

    // just update the node key and return it as the new root
    if (parent == nullptr) {
      node->key_nibbles = key_nibbles;
      return node;
    }

    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        return updateBranch(parent_as_branch, key_nibbles, node);
      }
      case T::Leaf: {
        // need to convert this leaf into a branch
        auto br = std::make_shared<BranchNode>();
        auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);

        if (parent->key_nibbles == key_nibbles
            && key_nibbles.size() == length) {
          return node;
        }

        br->key_nibbles = subbuffer(key_nibbles, 0, length);
        auto parentKey = parent->key_nibbles;

        // value goes at this branch
        if (key_nibbles.size() == length) {
          br->value = node->value;

          // if we are not replacing previous leaf, then add it as a
          // child to the new branch
          if (parent->key_nibbles.size() > key_nibbles.size()) {
            parent->key_nibbles = subbuffer(parent->key_nibbles, length + 1);
            br->children[parentKey[length]] = parent;
          }

          return br;
        }

        node->key_nibbles = subbuffer(key_nibbles, length + 1);

        if (length == parent->key_nibbles.size()) {
          // if leaf's key is covered by this branch, then make the leaf's
          // value the value at this branch
          br->value = parent->value;
          br->children[key_nibbles[length]] = node;
        } else {
          // otherwise, make the leaf a child of the branch and update its
          // partial key
          parent->key_nibbles = subbuffer(parent->key_nibbles, length + 1);
          br->children[parentKey[length]] = parent;
          br->children[key_nibbles[length]] = node;
        }

        return br;
      }
      default:
        return Error::INVALID_NODE_TYPE;
    }
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::updateBranch(
      BranchPtr parent, const common::Buffer &key_nibbles,
      const NodePtr &node) {
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);

    if (length == parent->key_nibbles.size()) {
      // just set the value in the parent to the node value
      if (key_nibbles == parent->key_nibbles) {
        parent->value = node->value;
        return parent;
      }
      OUTCOME_TRY(c, retrieveChild(parent, key_nibbles[length]));
      if (c) {
        OUTCOME_TRY(n, insert(c, subbuffer(key_nibbles, length + 1), node));
        parent->children[key_nibbles[length]] = n;
        return parent;
      } else {
        node->key_nibbles = subbuffer(key_nibbles, length + 1);
        parent->children[key_nibbles[length]] = node;
        return parent;
      }
    }
    auto br = std::make_shared<BranchNode>(subbuffer(key_nibbles, 0, length));
    auto parentIdx = parent->key_nibbles[length];
    OUTCOME_TRY(
        new_branch,
        insert(nullptr, subbuffer(parent->key_nibbles, length + 1), parent));
    br->children[parentIdx] = new_branch;
    if (key_nibbles.size() <= length) {
      br->value = node->value;
    } else {
      OUTCOME_TRY(new_child,
                  insert(nullptr, subbuffer(key_nibbles, length + 1), node));
      br->children[key_nibbles[length]] = new_child;
    }
    return br;
  }

  outcome::result<common::Buffer> PolkadotTrieDb::get(
      const common::Buffer &key) const {
    if (not root_.has_value()) {
      return common::Buffer{};
    }
    OUTCOME_TRY(root, retrieveNode(root_.value()));
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(node, getNode(root, nibbles));
    if (node) {
      return node->value;
    }
    return common::Buffer{};
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::getNode(
      NodePtr parent, const common::Buffer &key_nibbles) const {
    using T = PolkadotNode::Type;
    if (parent == nullptr) {
      return nullptr;
    }
    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        auto length = getCommonPrefixLength(parent->key_nibbles, key_nibbles);
        if (parent->key_nibbles == key_nibbles || key_nibbles.empty()) {
          auto foundLeaf =
              std::make_shared<LeafNode>(parent->key_nibbles, parent->value);
          return foundLeaf;
        }
        if ((subbuffer(parent->key_nibbles, 0, length) == key_nibbles)
            && key_nibbles.size() < parent->key_nibbles.size()) {
          return nullptr;
        }
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        OUTCOME_TRY(n, retrieveChild(parent_as_branch, key_nibbles[length]));
        return getNode(n, subbuffer(key_nibbles, length + 1));
      }
      case T::Leaf:
        if (parent->key_nibbles == key_nibbles) {
          return parent;
        }
        break;
      default:
        return Error::INVALID_NODE_TYPE;
    }
    return nullptr;
  }

  bool PolkadotTrieDb::contains(const common::Buffer &key) const {
    if (not root_.has_value())
      return false;
    auto root = retrieveNode(root_.value());
    if (root) {
      auto node = getNode(root.value(), PolkadotCodec::keyToNibbles(key));
      return node.has_value() && (node.value() != nullptr);
    }
    return false;
  }

  outcome::result<void> PolkadotTrieDb::remove(const common::Buffer &key) {
    if (root_.has_value()) {
      OUTCOME_TRY(root, retrieveNode(root_.value()));
      auto key_nibbles = PolkadotCodec::keyToNibbles(key);
      // delete node will fetch nodes that it needs from the storage (the nodes
      // typically are a path in the trie) and work on them in memory
      OUTCOME_TRY(n, deleteNode(root, key_nibbles));
      // afterwards, the nodes are written back to the storage and the new trie
      // root hash is obtained
      if (n == nullptr) {
        root_ = std::nullopt;
      } else {
        OUTCOME_TRY(root_hash, storeNode(*n));
        root_ = root_hash;
      }
    }
    return outcome::success();
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::deleteNode(
      NodePtr parent, const common::Buffer &key_nibbles) {
    if (!parent) {
      return nullptr;
    }
    using T = PolkadotNode::Type;
    auto newRoot = parent;
    switch (parent->getTrieType()) {
      case T::BranchWithValue:
      case T::BranchEmptyValue: {
        auto length = getCommonPrefixLength(parent->key_nibbles, key_nibbles);
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        if (parent->key_nibbles == key_nibbles || key_nibbles.empty()) {
          parent->value.clear();
          newRoot = parent;
        } else {
          OUTCOME_TRY(child,
                      retrieveChild(parent_as_branch, key_nibbles[length]));
          OUTCOME_TRY(n, deleteNode(child, subbuffer(key_nibbles, length + 1)));
          newRoot = parent;
          parent_as_branch->children[key_nibbles[length]] = n;
        }
        OUTCOME_TRY(n, handleDeletion(parent_as_branch, newRoot, key_nibbles));
        return n;
      }
      case T::Leaf:
        if (parent->key_nibbles == key_nibbles || key_nibbles.empty()) {
          return nullptr;
        }
        return parent;
      default:
        return Error::INVALID_NODE_TYPE;
    }
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::handleDeletion(
      const BranchPtr &parent, NodePtr node,
      const common::Buffer &key_nibbles) {
    auto newRoot = std::move(node);
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);
    auto bitmap = parent->childrenBitmap();
    // turn branch node left with no children to a leaf node
    if (bitmap == 0 && !parent->value.empty()) {
      newRoot = std::make_shared<LeafNode>(subbuffer(key_nibbles, 0, length),
                                           parent->value);
    } else if (parent->childrenNum() == 1 && parent->value.empty()) {
      size_t idx = 0;
      for (idx = 0; idx < 16; idx++) {
        bitmap >>= 1u;
        if (bitmap == 0) {
          break;
        }
      }
      OUTCOME_TRY(child, retrieveChild(parent, idx));
      using T = PolkadotNode::Type;
      if (child->getTrieType() == T::Leaf) {
        auto newKey = parent->key_nibbles;
        newKey.putUint8(idx);
        newKey.putBuffer(child->key_nibbles);
        newRoot = std::make_shared<LeafNode>(newKey, child->value);
      } else if (child->getTrieType() == T::BranchEmptyValue
                 || child->getTrieType() == T::BranchWithValue) {
        auto branch = std::make_shared<BranchNode>();
        branch->key_nibbles.putBuffer(parent->key_nibbles)
            .putUint8(idx)
            .putBuffer(child->key_nibbles);
        auto child_as_branch = std::dynamic_pointer_cast<BranchNode>(child);
        for (size_t i = 0; i < child_as_branch->children.size(); i++) {
          if (child_as_branch->children[i]) {
            branch->children[i] = child_as_branch->children[i];
          }
        }
        branch->value = child->value;
        newRoot = branch;
      }
    }
    return newRoot;
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::detachNode(
      const NodePtr &parent, const common::Buffer &prefix_nibbles) {
    if (parent == nullptr) {
      return nullptr;
    }
    if (parent->key_nibbles.size() >= prefix_nibbles.size()) {
      // if this is the node to be detached -- detach it
      if (subbuffer(parent->key_nibbles, 0, prefix_nibbles.size())
          == prefix_nibbles) {
        return nullptr;
      }
      return parent;
    }
    // if parent's key is smaller and it is not a prefix of the prefix, don't
    // change anything
    if (subbuffer(prefix_nibbles, 0, parent->key_nibbles.size())
        != parent->key_nibbles) {
      return parent;
    }
    using T = PolkadotNode::Type;
    if (parent->getTrieType() == T::BranchWithValue
        || parent->getTrieType() == T::BranchEmptyValue) {
      auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
      auto length = getCommonPrefixLength(parent->key_nibbles, prefix_nibbles);
      OUTCOME_TRY(child, retrieveChild(branch, prefix_nibbles[length]));
      if (child == nullptr) {
        return parent;
      }
      OUTCOME_TRY(n, detachNode(child, subbuffer(prefix_nibbles, length + 1)));
      branch->children[prefix_nibbles[length]] = n;
      return branch;
    }
    return parent;
  }

  uint32_t PolkadotTrieDb::getCommonPrefixLength(const Buffer &pref1,
                                                 const Buffer &pref2) const {
    uint32_t length = 0, min = pref1.size();

    if (pref1.size() > pref2.size()) {
      min = pref2.size();
    }

    for (; length < min; length++) {
      if (pref1[length] != pref2[length]) {
        break;
      }
    }

    return length;
  }

  outcome::result<common::Buffer> PolkadotTrieDb::storeNode(
      PolkadotNode &node) {
    auto batch = db_->batch();
    OUTCOME_TRY(hash, storeNode(node, *batch));
    OUTCOME_TRY(batch->commit());
    return hash;
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
      for (size_t i = 0; i < branch.children.size(); i++) {
        auto child = branch.children[i];
        if (child and not child->isDummy()) {
          OUTCOME_TRY(hash, storeNode(*child));
          // when a node is written to the storage, it is replaced with a dummy
          // node to avoid memory waste
          branch.children[i] = std::make_shared<DummyNode>(hash);
        }
      }
    }

    OUTCOME_TRY(enc, codec_.encodeNode(node));
    auto key = Buffer{codec_.hash256(enc)};
    OUTCOME_TRY(db_->put(key, enc));
    return key;
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::retrieveChild(
      const BranchPtr &parent, uint8_t idx) const {
    if (parent->children[idx] == nullptr) {
      return nullptr;
    }
    if (parent->children[idx]->isDummy()) {
      auto dummy = std::dynamic_pointer_cast<DummyNode>(parent->children[idx]);
      OUTCOME_TRY(n, retrieveNode(dummy->db_key));
      parent->children[idx] = n;
    }
    return parent->children[idx];
  }

  outcome::result<PolkadotTrieDb::NodePtr> PolkadotTrieDb::retrieveNode(
      const common::Buffer &db_key) const {
    OUTCOME_TRY(enc, db_->get(db_key));
    scale::ByteArrayStream s{enc};
    OUTCOME_TRY(n, codec_.decodeNode(s));
    return std::dynamic_pointer_cast<PolkadotNode>(n);
  }

}  // namespace kagome::storage::merkle

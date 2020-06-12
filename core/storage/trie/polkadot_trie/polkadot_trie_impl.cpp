/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"

#include <functional>
#include <utility>

#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

using kagome::common::Buffer;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, PolkadotTrieImpl::Error, e) {
  using E = kagome::storage::trie::PolkadotTrieImpl::Error;
  switch (e) {
    case E::INVALID_NODE_TYPE:
      return "The node type is invalid";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {
  PolkadotTrieImpl::PolkadotTrieImpl(ChildRetrieveFunctor f)
      : retrieve_child_{std::move(f)} {
    BOOST_ASSERT(retrieve_child_);
  }

  PolkadotTrieImpl::PolkadotTrieImpl(NodePtr root, ChildRetrieveFunctor f)
      : retrieve_child_{std::move(f)}, root_{std::move(root)} {
    BOOST_ASSERT(retrieve_child_);
  }

  outcome::result<void> PolkadotTrieImpl::put(const Buffer &key,
                                          const Buffer &value) {
    auto value_copy = value;
    return put(key, std::move(value_copy));
  }

  PolkadotTrie::NodePtr PolkadotTrieImpl::getRoot() const {
    return root_;
  }

  outcome::result<void> PolkadotTrieImpl::put(const Buffer &key, Buffer &&value) {
    auto k_enc = PolkadotCodec::keyToNibbles(key);

    NodePtr root = root_;

    // insert fetches a sequence of nodes (a path) from the storage and
    // these nodes are processed in memory, so any changes applied to them
    // will be written back to the storage only on storeNode call
    OUTCOME_TRY(n,
                insert(root, k_enc, std::make_shared<LeafNode>(k_enc, value)));
    root_ = n;

    return outcome::success();
  }

  outcome::result<void> PolkadotTrieImpl::clearPrefix(
      const common::Buffer &prefix) {
    if (not root_) {
      return outcome::success();
    }
    auto key_nibbles = PolkadotCodec::keyToNibbles(prefix);
    OUTCOME_TRY(new_root, detachNode(root_, key_nibbles));
    root_ = new_root;

    return outcome::success();
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::insert(
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
          node->key_nibbles = key_nibbles;
          return node;
        }

        br->key_nibbles = key_nibbles.subbuffer(0, length);
        auto parentKey = parent->key_nibbles;

        // value goes at this branch
        if (key_nibbles.size() == length) {
          br->value = node->value;

          // if we are not replacing previous leaf, then add it as a
          // child to the new branch
          if (parent->key_nibbles.size() > key_nibbles.size()) {
            parent->key_nibbles = parent->key_nibbles.subbuffer(length + 1);
            br->children.at(parentKey[length]) = parent;
          }

          return br;
        }

        node->key_nibbles = key_nibbles.subbuffer(length + 1);

        if (length == parent->key_nibbles.size()) {
          // if leaf's key is covered by this branch, then make the leaf's
          // value the value at this branch
          br->value = parent->value;
          br->children.at(key_nibbles[length]) = node;
        } else {
          // otherwise, make the leaf a child of the branch and update its
          // partial key
          parent->key_nibbles = parent->key_nibbles.subbuffer(length + 1);
          br->children.at(parentKey[length]) = parent;
          br->children.at(key_nibbles[length]) = node;
        }

        return br;
      }
      default:
        return Error::INVALID_NODE_TYPE;
    }
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::updateBranch(
      BranchPtr parent,
      const common::Buffer &key_nibbles,
      const NodePtr &node) {
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);

    if (length == parent->key_nibbles.size()) {
      // just set the value in the parent to the node value
      if (key_nibbles == parent->key_nibbles) {
        parent->value = node->value;
        return parent;
      }
      OUTCOME_TRY(child, retrieveChild(parent, key_nibbles[length]));
      if (child) {
        OUTCOME_TRY(n, insert(child, key_nibbles.subbuffer(length + 1), node));
        parent->children.at(key_nibbles[length]) = n;
        return parent;
      }
      node->key_nibbles = key_nibbles.subbuffer(length + 1);
      parent->children.at(key_nibbles[length]) = node;
      return parent;
    }
    auto br = std::make_shared<BranchNode>(key_nibbles.subbuffer(0, length));
    auto parentIdx = parent->key_nibbles[length];
    OUTCOME_TRY(
        new_branch,
        insert(nullptr, parent->key_nibbles.subbuffer(length + 1), parent));
    br->children.at(parentIdx) = new_branch;
    if (key_nibbles.size() <= length) {
      br->value = node->value;
    } else {
      OUTCOME_TRY(new_child,
                  insert(nullptr, key_nibbles.subbuffer(length + 1), node));
      br->children.at(key_nibbles[length]) = new_child;
    }
    return br;
  }

  outcome::result<common::Buffer> PolkadotTrieImpl::get(
      const common::Buffer &key) const {
    if (not root_) {
      return TrieError::NO_VALUE;
    }
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(node, getNode(root_, nibbles));
    if (node && node->value) {
      return node->value.get();
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::getNode(
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
          auto found_leaf =
              std::make_shared<LeafNode>(parent->key_nibbles, parent->value);
          return found_leaf;
        }
        if ((parent->key_nibbles.subbuffer(0, length) == key_nibbles)
            && key_nibbles.size() < parent->key_nibbles.size()) {
          return nullptr;
        }
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        OUTCOME_TRY(n, retrieveChild(parent_as_branch, key_nibbles[length]));
        return getNode(n, key_nibbles.subbuffer(length + 1));
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

  std::unique_ptr<BufferMapCursor> PolkadotTrieImpl::cursor() {
    return std::make_unique<PolkadotTrieCursor>(*this);
  }

  bool PolkadotTrieImpl::contains(const common::Buffer &key) const {
    if (not root_) {
      return false;
    }

    auto node = getNode(root_, PolkadotCodec::keyToNibbles(key));
    return node.has_value() && (node.value() != nullptr)
           && (node.value()->value);
  }

  bool PolkadotTrieImpl::empty() const {
    return root_ == nullptr;
  }

  outcome::result<void> PolkadotTrieImpl::remove(const common::Buffer &key) {
    if (root_) {
      auto key_nibbles = PolkadotCodec::keyToNibbles(key);
      // delete node will fetch nodes that it needs from the storage (the nodes
      // typically are a path in the trie) and work on them in memory
      OUTCOME_TRY(n, deleteNode(root_, key_nibbles));
      // afterwards, the nodes are written back to the storage and the new trie
      // root hash is obtained
      root_ = n;
    }
    return outcome::success();
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::deleteNode(
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
          parent->value = boost::none;
          newRoot = parent;
        } else {
          OUTCOME_TRY(child,
                      retrieveChild(parent_as_branch, key_nibbles[length]));
          OUTCOME_TRY(n, deleteNode(child, key_nibbles.subbuffer(length + 1)));
          newRoot = parent;
          parent_as_branch->children.at(key_nibbles[length]) = n;
        }
        OUTCOME_TRY(n, handleDeletion(parent_as_branch, newRoot, key_nibbles));
        return std::move(n);
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

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::handleDeletion(
      const BranchPtr &parent,
      NodePtr node,
      const common::Buffer &key_nibbles) {
    auto newRoot = std::move(node);
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);
    auto bitmap = parent->childrenBitmap();
    // turn branch node left with no children to a leaf node
    if (bitmap == 0 && parent->value) {
      newRoot = std::make_shared<LeafNode>(key_nibbles.subbuffer(0, length),
                                           parent->value);
    } else if (parent->childrenNum() == 1 && !parent->value) {
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
          if (child_as_branch->children.at(i)) {
            branch->children.at(i) = child_as_branch->children.at(i);
          }
        }
        branch->value = child->value;
        newRoot = branch;
      }
    }
    return newRoot;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::detachNode(
      const NodePtr &parent, const common::Buffer &prefix_nibbles) {
    if (parent == nullptr) {
      return nullptr;
    }
    if (parent->key_nibbles.size() >= prefix_nibbles.size()) {
      // if this is the node to be detached -- detach it
      if (parent->key_nibbles.subbuffer(0, prefix_nibbles.size())
          == prefix_nibbles) {
        return nullptr;
      }
      return parent;
    }
    // if parent's key is smaller and it is not a prefix of the prefix, don't
    // change anything
    if (prefix_nibbles.subbuffer(0, parent->key_nibbles.size())
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
      OUTCOME_TRY(n, detachNode(child, prefix_nibbles.subbuffer(length + 1)));
      branch->children.at(prefix_nibbles[length]) = n;
      return branch;
    }
    return parent;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      BranchPtr parent, uint8_t idx) const {
    return retrieve_child_(std::move(parent), idx);
  }

  uint32_t PolkadotTrieImpl::getCommonPrefixLength(const Buffer &pref1,
                                               const Buffer &pref2) const {
    size_t length = 0;
    auto min = pref1.size();

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

}  // namespace kagome::storage::trie

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"

#include <functional>
#include <utility>

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

using kagome::common::Buffer;

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, PolkadotTrieImpl::Error, e) {
  using E = kagome::storage::trie::PolkadotTrieImpl::Error;
  switch (e) {
    case E::INVALID_NODE_TYPE:
      return "The trie node type is invalid";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  uint32_t getCommonPrefixLength(const KeyNibbles &, const KeyNibbles &);

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

  outcome::result<void> PolkadotTrieImpl::put(const Buffer &key,
                                              Buffer &&value) {
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

  outcome::result<size_t> detachNode(PolkadotTrie::NodePtr &,
                                     const KeyNibbles &,
                                     const PolkadotTrie::OnDetachCallback &);

  outcome::result<std::tuple<bool, uint32_t>> PolkadotTrieImpl::clearPrefix(
      const common::Buffer &prefix,
      boost::optional<uint64_t> limit,
      const OnDetachCallback &callback) {
    auto key_nibbles = PolkadotCodec::keyToNibbles(prefix);
    OUTCOME_TRY(size, detachNode(root_, key_nibbles, callback));

    return outcome::success(std::make_tuple(true, size));
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::insert(
      const NodePtr &parent, const KeyNibbles &key_nibbles, NodePtr node) {
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
      BranchPtr parent, const KeyNibbles &key_nibbles, const NodePtr &node) {
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);

    if (length == parent->key_nibbles.size()) {
      // just set the value in the parent to the node value
      if (key_nibbles == parent->key_nibbles) {
        parent->value = node->value;
        return parent;
      }
      OUTCOME_TRY(child, retrieveChild(parent, key_nibbles[length]));
      if (child) {
        OUTCOME_TRY(n, insert(child, key_nibbles.subspan(length + 1), node));
        parent->children.at(key_nibbles[length]) = n;
        return parent;
      }
      node->key_nibbles = key_nibbles.subbuffer(length + 1);
      parent->children.at(key_nibbles[length]) = node;
      return parent;
    }
    auto br = std::make_shared<BranchNode>(key_nibbles.subspan(0, length));
    auto parentIdx = parent->key_nibbles[length];
    OUTCOME_TRY(
        new_branch,
        insert(nullptr, parent->key_nibbles.subspan(length + 1), parent));
    br->children.at(parentIdx) = new_branch;
    if (key_nibbles.size() <= length) {
      br->value = node->value;
    } else {
      OUTCOME_TRY(new_child,
                  insert(nullptr, key_nibbles.subspan(length + 1), node));
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
      NodePtr parent, const KeyNibbles &key_nibbles) const {
    using T = PolkadotNode::Type;
    if (parent == nullptr) {
      return nullptr;
    }
    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        if (parent->key_nibbles == key_nibbles or key_nibbles.empty()) {
          return parent;
        }
        if (key_nibbles.size() < parent->key_nibbles.size()) {
          return nullptr;
        }
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        auto length = getCommonPrefixLength(parent->key_nibbles, key_nibbles);
        OUTCOME_TRY(n, retrieveChild(parent_as_branch, key_nibbles[length]));
        return getNode(n, key_nibbles.subspan(length + 1));
      }
      case T::Leaf:
        if (parent->key_nibbles == key_nibbles) {
          return parent;
        }
        break;
      case T::Special:
        return Error::INVALID_NODE_TYPE;
    }
    return nullptr;
  }

  outcome::result<std::list<std::pair<PolkadotTrieImpl::BranchPtr, uint8_t>>>
  PolkadotTrieImpl::getPath(NodePtr parent,
                            const KeyNibbles &key_nibbles) const {
    using Path = std::list<std::pair<PolkadotTrieImpl::BranchPtr, uint8_t>>;
    using T = PolkadotNode::Type;
    if (parent == nullptr) {
      return TrieError::NO_VALUE;
    }
    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        auto length = getCommonPrefixLength(parent->key_nibbles, key_nibbles);
        if (parent->key_nibbles == key_nibbles or key_nibbles.empty()) {
          return Path{};
        }
        auto common_nibbles =
            gsl::make_span(parent->key_nibbles.data(), length);
        if (key_nibbles == common_nibbles
            and key_nibbles.size() < parent->key_nibbles.size()) {
          return Path{};
        }
        auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
        OUTCOME_TRY(n, retrieveChild(parent_as_branch, key_nibbles[length]));
        OUTCOME_TRY(path, getPath(n, key_nibbles.subspan(length + 1)));
        path.push_front({parent_as_branch, key_nibbles[length]});
        return std::move(path);
      }
      case T::Leaf:
        if (parent->key_nibbles == key_nibbles) {
          return Path{};
        }
        break;
      default:
        return Error::INVALID_NODE_TYPE;
    }
    return TrieError::NO_VALUE;
  }

  std::unique_ptr<PolkadotTrieCursor> PolkadotTrieImpl::trieCursor() {
    return std::make_unique<PolkadotTrieCursorImpl>(*this);
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

  void deleteNode(PolkadotTrie::NodePtr &, const KeyNibbles &);

  outcome::result<void> PolkadotTrieImpl::remove(const common::Buffer &key) {
    auto key_nibbles = PolkadotCodec::keyToNibbles(key);
    // delete node will fetch nodes that it needs from the storage (the
    // nodes typically are a path in the trie) and work on them in memory
    deleteNode(root_, key_nibbles);
    return outcome::success();
  }

  void handleDeletion(PolkadotTrie::NodePtr &);

  void deleteNode(PolkadotTrie::NodePtr &parent, const KeyNibbles &key) {
    if (parent == nullptr) {
      return;
    }
    if (parent->isBranch()) {
      auto length = getCommonPrefixLength(parent->key_nibbles, key);
      auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
      if (parent->key_nibbles == key or key.empty()) {
        parent->value = boost::none;
      } else {
        auto &child = branch->children.at(key[length]);
        deleteNode(child, key.subspan(length + 1));
      }
      handleDeletion(parent);
    } else if (parent->key_nibbles == key or key.empty()) {
      parent.reset();
    }
  }

  void handleDeletion(PolkadotTrie::NodePtr &parent) {
    auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
    auto bitmap = branch->childrenBitmap();
    // turn branch node left with no children to a leaf node
    if (bitmap == 0 and branch->value) {
      parent =
          std::make_shared<LeafNode>(parent->key_nibbles, branch->value);
    } else if (branch->childrenNum() == 1 && !branch->value) {
      size_t idx = 0;
      for (idx = 0; idx < 16; idx++) {
        bitmap >>= 1u;
        if (bitmap == 0) {
          break;
        }
      }
      auto child = branch->children.at(idx);
      using T = PolkadotNode::Type;
      if (child->getTrieType() == T::Leaf) {
        parent = std::make_shared<LeafNode>(parent->key_nibbles, child->value);
      } else if (child->isBranch()) {
        branch->children.at(idx).reset();
        auto child_as_branch = std::dynamic_pointer_cast<BranchNode>(child);
        branch->children = child_as_branch->children;
        branch->value = child->value;
      }
      parent->key_nibbles.putUint8(idx).putBuffer(child->key_nibbles);
    }
  }

  outcome::result<size_t> notifyIsDetached(
      const PolkadotTrie::NodePtr &, const PolkadotTrie::OnDetachCallback &);

  outcome::result<size_t> detachNode(
      PolkadotTrie::NodePtr &parent,
      const KeyNibbles &prefix,
      const PolkadotTrie::OnDetachCallback &callback) {
    size_t count = 0;

    if (parent == nullptr) {
      return count;
    }

    if (parent->key_nibbles.size() >= prefix.size()) {
      // if this is the node to be detached -- detach it
      if (std::equal(
              prefix.begin(), prefix.end(), parent->key_nibbles.begin())) {
        OUTCOME_TRY(size, notifyIsDetached(parent, callback));
        parent.reset();
        return count + size;
      }
      return count;
    }

    // if parent's key is smaller and it is not a prefix of the prefix, don't
    // change anything
    if (not std::equal(parent->key_nibbles.begin(),
                       parent->key_nibbles.end(),
                       prefix.begin())) {
      return count;
    }

    if (parent->isBranch()) {
      const auto length = parent->key_nibbles.size();
      auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
      auto &child = branch->children.at(prefix[length]);

      OUTCOME_TRY(size, detachNode(child, prefix.subspan(length + 1), callback));
      handleDeletion(parent);
      return count + size;
    }

    return count;
  }

  outcome::result<size_t> notifyIsDetached(
      const PolkadotTrie::NodePtr &node,
      const PolkadotTrie::OnDetachCallback &callback) {
    size_t count = 0;
    if (node) {
      if (node->isBranch()) {
        auto branch = std::dynamic_pointer_cast<BranchNode>(node);
        for (auto &child : branch->children) {
          OUTCOME_TRY(size, notifyIsDetached(child, callback));
          count += size;
        }
      }

      auto key = PolkadotCodec::nibblesToKey(node->key_nibbles);
      // count only valued nodes
      if(node->value) {
        ++count;
      }
      OUTCOME_TRY(callback(key, std::move(node->value)));
    }
    return outcome::success(count);
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      BranchPtr parent, uint8_t idx) const {
    return retrieve_child_(std::move(parent), idx);
  }

  uint32_t getCommonPrefixLength(const KeyNibbles &first,
                                 const KeyNibbles &second) {
    auto &&[it, _] =
        std::mismatch(first.begin(), first.end(), second.begin(), second.end());
    return it - first.begin();
  }

}  // namespace kagome::storage::trie

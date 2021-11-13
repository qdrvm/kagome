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

namespace {
  using namespace kagome::storage::trie;

  uint32_t getCommonPrefixLength(const KeyNibbles &first,
                                 const KeyNibbles &second) {
    auto &&[it, _] =
        std::mismatch(first.begin(), first.end(), second.begin(), second.end());
    return it - first.begin();
  }

  /**
   * Fixes node type, merge nodes after children removal
   * 1. if node has no children left, but has value, then turn to leaf.
   * 2. if node has no children and no value (can't really happen anywhere but
   clearPrefix), then reset node.
   * 3. if node has 1 child and is not a value, merge it with this child, update
   its key, and copy children if any
   */
  outcome::result<void> handleDeletion(
      PolkadotTrie::NodePtr &parent,
      const PolkadotTrieImpl::NodeRetrieveFunctor &retrieveNode) {
    auto branch = std::dynamic_pointer_cast<BranchNode>(parent);
    auto bitmap = branch->childrenBitmap();
    if (bitmap == 0) {
      if (parent->value) {
        // turn branch node left with no children to a leaf node
        parent = std::make_shared<LeafNode>(parent->key_nibbles, parent->value);
      } else {
        // this case actual only for clearPrefix, unreal situation in deletion
        parent.reset();
      }
    } else if (branch->childrenNum() == 1 && !parent->value) {
      size_t idx = 0;
      while (bitmap >>= 1u) ++idx;
      auto child = branch->children.at(idx);

      OUTCOME_TRY(retrieveNode(child));

      using T = PolkadotNode::Type;
      if (child->getTrieType() == T::Leaf) {
        parent = std::make_shared<LeafNode>(parent->key_nibbles, child->value);
      } else if (child->isBranch()) {
        branch->children = dynamic_cast<BranchNode &>(*child.get()).children;
        parent->value = child->value;
      }
      parent->key_nibbles.putUint8(idx).putBuffer(child->key_nibbles);
    }
    return outcome::success();
  }

  outcome::result<void> deleteNode(
      PolkadotTrie::NodePtr &parent,
      const KeyNibbles &key,
      const PolkadotTrieImpl::NodeRetrieveFunctor &retrieveNode) {
    if (parent == nullptr) {
      return outcome::success();
    }

    OUTCOME_TRY(retrieveNode(parent));

    if (parent->isBranch()) {
      if (parent->key_nibbles == key) {
        parent->value = std::nullopt;
      } else {
        auto length = getCommonPrefixLength(parent->key_nibbles, key);
        auto &child =
            dynamic_cast<BranchNode &>(*parent.get()).children.at(key[length]);
        OUTCOME_TRY(deleteNode(child, key.subspan(length + 1), retrieveNode));
      }
      OUTCOME_TRY(handleDeletion(parent, retrieveNode));
    } else if (parent->key_nibbles == key) {
      parent.reset();
    }
    return outcome::success();
  }

  outcome::result<void> notifyOnDetached(
      PolkadotTrie::NodePtr &node,
      const PolkadotTrie::OnDetachCallback &callback) {
    auto key = PolkadotCodec::nibblesToKey(node->key_nibbles);
    OUTCOME_TRY(callback(key, std::move(node->value)));
    return outcome::success();
  }

  /**
   * remove a node with its children
   * @param limit is a max number of values to remove
   * @param finished is true if all values removed, false if limit exceeded
   * @param count is a number of values deleted
   */
  outcome::result<void> detachNode(
      PolkadotTrie::NodePtr &parent,
      const KeyNibbles &prefix,
      std::optional<uint64_t> limit,
      bool &finished,
      uint32_t &count,
      const PolkadotTrie::OnDetachCallback &callback,
      const PolkadotTrieImpl::NodeRetrieveFunctor &retrieveNode) {
    if (parent == nullptr) {
      return outcome::success();
    }

    // means 'limit ended'
    if (not finished) {
      return outcome::success();
    }

    OUTCOME_TRY(retrieveNode(parent));

    if (parent->key_nibbles.size() >= prefix.size()) {
      // if this is the node to be detached -- detach it
      if (std::equal(
              prefix.begin(), prefix.end(), parent->key_nibbles.begin())) {
        // remove all children one by one according to limit
        if (parent->isBranch()) {
          auto &children = dynamic_cast<BranchNode &>(*parent.get()).children;
          for (auto &child : children) {
            OUTCOME_TRY(detachNode(child,
                                   KeyNibbles(),
                                   limit,
                                   finished,
                                   count,
                                   callback,
                                   retrieveNode));
          }
        }
        if (not limit or count < limit.value()) {
          if (parent->value) {
            OUTCOME_TRY(notifyOnDetached(parent, callback));
            ++count;
          }
          parent.reset();
        } else {
          if (parent->value) {
            // we saw a value after limit, so not finished
            finished = false;
          }
          if (parent->isBranch()) {
            // fix block after children removal
            OUTCOME_TRY(handleDeletion(parent, retrieveNode));
          }
        }
        return outcome::success();
      }
    }

    // if parent's key is smaller and it is not a prefix of the prefix, don't
    // change anything
    if (not std::equal(parent->key_nibbles.begin(),
                       parent->key_nibbles.end(),
                       prefix.begin())) {
      return outcome::success();
    }

    if (parent->isBranch()) {
      const auto length = parent->key_nibbles.size();
      auto &child =
          dynamic_cast<BranchNode &>(*parent.get()).children.at(prefix[length]);

      OUTCOME_TRY(detachNode(child,
                             prefix.subspan(length + 1),
                             limit,
                             finished,
                             count,
                             callback,
                             retrieveNode));
      OUTCOME_TRY(handleDeletion(parent, retrieveNode));
    }
    return outcome::success();
  }

}  // namespace

namespace kagome::storage::trie {

  PolkadotTrieImpl::PolkadotTrieImpl(NodeRetrieveFunctor f)
      : retrieve_node_{std::move(f)} {
    BOOST_ASSERT(retrieve_node_);
  }

  PolkadotTrieImpl::PolkadotTrieImpl(NodePtr root, NodeRetrieveFunctor f)
      : retrieve_node_{std::move(f)}, root_{std::move(root)} {
    BOOST_ASSERT(retrieve_node_);
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

  outcome::result<std::tuple<bool, uint32_t>> PolkadotTrieImpl::clearPrefix(
      const common::Buffer &prefix,
      std::optional<uint64_t> limit,
      const OnDetachCallback &callback) {
    bool finished = true;
    uint32_t count = 0;
    auto key_nibbles = PolkadotCodec::keyToNibbles(prefix);
    OUTCOME_TRY(detachNode(
        root_, key_nibbles, limit, finished, count, callback, retrieve_node_));
    return {finished, count};
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
    OUTCOME_TRY(opt_value, tryGet(key));
    if (opt_value.has_value()) {
      return opt_value.value();
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<std::optional<common::Buffer>> PolkadotTrieImpl::tryGet(
      const common::Buffer &key) const {
    if (not root_) {
      return std::nullopt;
    }
    auto nibbles = PolkadotCodec::keyToNibbles(key);
    OUTCOME_TRY(node, getNode(root_, nibbles));
    if (node && node->value) {
      return node->value.value();
    }
    return std::nullopt;
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

  outcome::result<bool> PolkadotTrieImpl::contains(
      const common::Buffer &key) const {
    if (not root_) {
      return false;
    }

    OUTCOME_TRY(node, getNode(root_, PolkadotCodec::keyToNibbles(key)));
    return node != nullptr && node->value;
  }

  bool PolkadotTrieImpl::empty() const {
    return root_ == nullptr;
  }

  outcome::result<void> PolkadotTrieImpl::remove(const common::Buffer &key) {
    auto key_nibbles = PolkadotCodec::keyToNibbles(key);
    // delete node will fetch nodes that it needs from the storage (the
    // nodes typically are a path in the trie) and work on them in memory
    return deleteNode(root_, key_nibbles, retrieve_node_);
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      BranchPtr parent, uint8_t idx) const {
    auto &child = parent->children.at(idx);
    OUTCOME_TRY(retrieve_node_(child));
    return child;
  }

}  // namespace kagome::storage::trie

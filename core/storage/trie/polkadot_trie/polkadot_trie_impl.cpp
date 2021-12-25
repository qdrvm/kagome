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

  uint32_t getCommonPrefixLength(const NibblesView &first,
                                 const NibblesView &second) {
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

      OUTCOME_TRY(c, retrieveNode(child));

      using T = TrieNode::Type;
      if (c->getTrieType() == T::Leaf) {
        parent = std::make_shared<LeafNode>(parent->key_nibbles, c->value);
      } else if (c->isBranch()) {
        branch->children = dynamic_cast<BranchNode &>(*c.get()).children;
        parent->value = c->value;
      }
      parent->key_nibbles.putUint8(idx).putBuffer(c->key_nibbles);
    }
    return outcome::success();
  }

  outcome::result<bool> deleteNode(
      const std::shared_ptr<OpaqueTrieNode> &parent_handle,
      const NibblesView &key,
      const PolkadotTrieImpl::NodeRetrieveFunctor &retrieveNode) {
    if (parent_handle == nullptr) {
      return outcome::success();
    }

    OUTCOME_TRY(parent, retrieveNode(parent_handle));

    if (auto parent_as_branch = dynamic_cast<BranchNode *>(parent.get());
        parent_as_branch != nullptr) {
      if (parent->key_nibbles == key) {
        parent->value = std::nullopt;
      } else {
        auto length = getCommonPrefixLength(parent->key_nibbles, key);
        auto &child = parent_as_branch->children.at(key[length]);
        OUTCOME_TRY(deleted,
                    deleteNode(child, key.subspan(length + 1), retrieveNode));
        if (deleted) {
          parent_as_branch->children[key[length]] = nullptr;
        }
      }
      OUTCOME_TRY(handleDeletion(parent, retrieveNode));
    } else if (parent->key_nibbles == key) {
      parent.reset();
    }
    return outcome::success(true);
  }

  outcome::result<void> notifyOnDetached(
      const std::shared_ptr<TrieNode> &node,
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
      const std::shared_ptr<OpaqueTrieNode> &parent_handle,
      const NibblesView &prefix,
      std::optional<uint64_t> limit,
      bool &finished,
      uint32_t &count,
      const PolkadotTrie::OnDetachCallback &callback,
      const PolkadotTrieImpl::NodeRetrieveFunctor &retrieveNode) {
    if (parent_handle == nullptr) {
      return outcome::success();
    }

    // means 'limit ended'
    if (not finished) {
      return outcome::success();
    }

    OUTCOME_TRY(parent, retrieveNode(parent_handle));

    if (static_cast<ssize_t>(parent->key_nibbles.size()) >= prefix.size()) {
      // if this is the node to be detached -- detach it
      if (std::equal(
              prefix.begin(), prefix.end(), parent->key_nibbles.begin())) {
        // remove all children one by one according to limit
        if (parent->isBranch()) {
          auto &children = dynamic_cast<BranchNode &>(*parent.get()).children;
          for (auto &child : children) {
            OUTCOME_TRY(detachNode(child,
                                   NibblesView{},
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

  outcome::result<void> PolkadotTrieImpl::put(const BufferView &key,
                                              const Buffer &value) {
    auto value_copy = value;
    return put(key, std::move(value_copy));
  }

  PolkadotTrie::ConstNodePtr PolkadotTrieImpl::getRoot() const {
    return root_;
  }

  PolkadotTrie::NodePtr PolkadotTrieImpl::getRoot() {
    return root_;
  }

  outcome::result<void> PolkadotTrieImpl::put(const BufferView &key,
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
      const common::BufferView &prefix,
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
      const NodePtr &parent, const NibblesView &key_nibbles, NodePtr node) {
    using T = TrieNode::Type;

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
          node->key_nibbles = KeyNibbles{key_nibbles};
          return node;
        }

        br->key_nibbles = KeyNibbles{key_nibbles.subspan(0, length)};
        auto parentKey = parent->key_nibbles;

        // value goes at this branch
        if (key_nibbles.size() == length) {
          br->value = node->value;

          // if we are not replacing previous leaf, then add it as a
          // child to the new branch
          if (static_cast<ssize_t>(parent->key_nibbles.size())
              > key_nibbles.size()) {
            parent->key_nibbles = parent->key_nibbles.subbuffer(length + 1);
            br->children.at(parentKey[length]) = parent;
          }

          return br;
        }

        node->key_nibbles = KeyNibbles{key_nibbles.subspan(length + 1)};

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
      BranchPtr parent, const NibblesView &key_nibbles, const NodePtr &node) {
    auto length = getCommonPrefixLength(key_nibbles, parent->key_nibbles);

    if (length == parent->key_nibbles.size()) {
      // just set the value in the parent to the node value
      if (key_nibbles == parent->key_nibbles) {
        parent->value = node->value;
        return parent;
      }
      OUTCOME_TRY(child, retrieveChild(*parent, key_nibbles[length]));
      if (child) {
        OUTCOME_TRY(n, insert(child, key_nibbles.subspan(length + 1), node));
        parent->children.at(key_nibbles[length]) = n;
        return parent;
      }
      node->key_nibbles = KeyNibbles{key_nibbles.subspan(length + 1)};
      parent->children.at(key_nibbles[length]) = node;
      return parent;
    }
    auto br = std::make_shared<BranchNode>(
        KeyNibbles{key_nibbles.subspan(0, length)});
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

  outcome::result<common::BufferConstRef> PolkadotTrieImpl::get(
      const common::BufferView &key) const {
    OUTCOME_TRY(opt_value, tryGet(key));
    if (opt_value.has_value()) {
      return opt_value.value();
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<std::optional<common::BufferConstRef>>
  PolkadotTrieImpl::tryGet(const common::BufferView &key) const {
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
      ConstNodePtr parent, const NibblesView &key_nibbles) {
    OUTCOME_TRY(node,
                const_cast<const PolkadotTrieImpl *>(this)->getNode(
                    parent, key_nibbles));
    return std::const_pointer_cast<TrieNode>(node);
  }

  outcome::result<PolkadotTrie::ConstNodePtr> PolkadotTrieImpl::getNode(
      ConstNodePtr parent, const NibblesView &key_nibbles) const {
    using T = TrieNode::Type;
    if (parent == nullptr) {
      return nullptr;
    }
    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        if (parent->key_nibbles == key_nibbles or key_nibbles.empty()) {
          return parent;
        }
        if (key_nibbles.size()
            < static_cast<ssize_t>(parent->key_nibbles.size())) {
          return nullptr;
        }
        auto parent_as_branch =
            std::dynamic_pointer_cast<const BranchNode>(parent);
        auto length = getCommonPrefixLength(parent->key_nibbles, key_nibbles);
        if (parent_as_branch->children[key_nibbles[length]] == nullptr) {
          return nullptr;
        }
        OUTCOME_TRY(n, retrieveChild(*parent_as_branch, key_nibbles[length]));
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

  outcome::result<void> PolkadotTrieImpl::forNodeInPath(
      ConstNodePtr parent,
      const NibblesView &path,
      const std::function<outcome::result<void>(BranchNode const &,
                                                uint8_t idx)> &callback) const {
    using T = TrieNode::Type;
    if (parent == nullptr) {
      return TrieError::NO_VALUE;
    }
    switch (parent->getTrieType()) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        // path is completely covered by the parent key
        if (parent->key_nibbles == path or path.empty()) {
          return outcome::success();
        }
        auto common_length = getCommonPrefixLength(parent->key_nibbles, path);
        auto common_nibbles =
            gsl::make_span(parent->key_nibbles.data(), common_length);
        // path is even less than the parent key (path is the prefix of the
        // parent key)
        if (path == common_nibbles
            and path.size()
                    < static_cast<ssize_t>(parent->key_nibbles.size())) {
          return outcome::success();
        }
        auto parent_as_branch =
            std::dynamic_pointer_cast<const BranchNode>(parent);
        OUTCOME_TRY(child,
                    retrieveChild(*parent_as_branch, path[common_length]));
        OUTCOME_TRY(callback(*parent_as_branch, path[common_length]));
        return forNodeInPath(child, path.subspan(common_length + 1), callback);
      }
      case T::Leaf:
        if (parent->key_nibbles == path) {
          return outcome::success();
        }
        break;
      default:
        return Error::INVALID_NODE_TYPE;
    }
    return TrieError::NO_VALUE;
  }

  std::unique_ptr<PolkadotTrieCursor> PolkadotTrieImpl::trieCursor() {
    return std::make_unique<PolkadotTrieCursorImpl>(shared_from_this());
  }

  outcome::result<bool> PolkadotTrieImpl::contains(
      const common::BufferView &key) const {
    if (not root_) {
      return false;
    }

    OUTCOME_TRY(node, getNode(root_, PolkadotCodec::keyToNibbles(key)));
    return node != nullptr && node->value;
  }

  bool PolkadotTrieImpl::empty() const {
    return root_ == nullptr;
  }

  outcome::result<void> PolkadotTrieImpl::remove(
      const common::BufferView &key) {
    auto key_nibbles = PolkadotCodec::keyToNibbles(key);
    // delete node will fetch nodes that it needs from the storage (the
    // nodes typically are a path in the trie) and work on them in memory
    OUTCOME_TRY(res, deleteNode(root_, key_nibbles, retrieve_node_));
    return outcome::success();
  }

  outcome::result<PolkadotTrie::ConstNodePtr> PolkadotTrieImpl::retrieveChild(
      const BranchNode &parent, uint8_t idx) const {
    auto &maybe_dummy_child = parent.children.at(idx);
    OUTCOME_TRY(child, retrieve_node_(maybe_dummy_child));
    // replacing a dummy node to an actual node should be transparent to the
    // outside world
    const_cast<BranchNode &>(parent).children[idx] = child;
    return child;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      const BranchNode &parent, uint8_t idx) {
    auto &maybe_dummy_child = parent.children.at(idx);
    OUTCOME_TRY(child, retrieve_node_(maybe_dummy_child));
    // replacing a dummy node to an actual node should be transparent to the
    // outside world
    const_cast<BranchNode &>(parent).children[idx] = child;
    return child;
  }

}  // namespace kagome::storage::trie

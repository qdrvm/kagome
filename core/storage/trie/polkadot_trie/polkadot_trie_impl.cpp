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

  class OpaqueNodeStorage final {
   public:
    OpaqueNodeStorage(PolkadotTrie::NodeRetrieveFunctor node_retriever,
                      std::shared_ptr<TrieNode> root) noexcept
        : retrieve_node_{std::move(node_retriever)}, root_{std::move(root)} {}

    static outcome::result<std::unique_ptr<OpaqueNodeStorage>> createAt(
        std::shared_ptr<OpaqueTrieNode> root,
        PolkadotTrie::NodeRetrieveFunctor node_retriever) {
      OUTCOME_TRY(root_node, node_retriever(root));
      return std::unique_ptr<OpaqueNodeStorage>{
          new OpaqueNodeStorage{node_retriever, root_node}};
    }

    [[nodiscard]] const std::shared_ptr<TrieNode> &getRoot() {
      return root_;
    }

    [[nodiscard]] std::shared_ptr<const TrieNode> getRoot() const {
      return root_;
    }

    void setRoot(const std::shared_ptr<TrieNode> &root) {
      root_ = root;
    }

    [[nodiscard]] outcome::result<std::shared_ptr<const TrieNode>> getChild(
        BranchNode const &parent, uint8_t idx) const {
      // SAFETY: changing a parent's opaque child node from a handle to a node
      // to the actual node doesn't break it's const correctness, because opaque
      // nodes are meant to hide their content
      auto &mut_parent = const_cast<BranchNode &>(parent);
      auto &opaque_child = parent.children.at(idx);
      OUTCOME_TRY(child, retrieve_node_(opaque_child));
      mut_parent.children.at(idx) = child;
      return std::move(child);
    }

    [[nodiscard]] outcome::result<std::shared_ptr<TrieNode>> getChild(
        BranchNode const &parent, uint8_t idx) {
      // SAFETY: adding constness, not removing
      auto const_this = const_cast<const OpaqueNodeStorage *>(this);
      OUTCOME_TRY(const_child, const_this->getChild(parent, idx));
      // SAFETY: removing constness we manually added
      auto child = std::const_pointer_cast<TrieNode>(const_child);
      return child;
    }

   private:
    PolkadotTrie::NodeRetrieveFunctor retrieve_node_;
    std::shared_ptr<TrieNode> root_;
  };
}  // namespace kagome::storage::trie

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
  [[nodiscard]] outcome::result<void> handleDeletion(
      const kagome::log::Logger &logger,
      PolkadotTrie::NodePtr &parent,
      OpaqueNodeStorage &node_storage) {
    if (!parent->isBranch()) return outcome::success();
    auto &branch = dynamic_cast<BranchNode &>(*parent);
    auto bitmap = branch.childrenBitmap();
    if (bitmap == 0) {
      if (parent->value) {
        // turn branch node left with no children to a leaf node
        parent = std::make_shared<LeafNode>(parent->key_nibbles, parent->value);
        SL_TRACE(logger, "handleDeletion: turn childless branch into a leaf");
      } else {
        // this case actual only for clearPrefix, unreal situation in deletion
        parent = nullptr;
        SL_TRACE(logger, "handleDeletion: nullify valueless branch parent");
      }
    } else if (branch.childrenNum() == 1 && !branch.value) {
      size_t idx = 0;
      while (bitmap >>= 1u) ++idx;
      OUTCOME_TRY(child, node_storage.getChild(branch, idx));

      using T = TrieNode::Type;
      if (child->getTrieType() == T::Leaf) {
        parent = std::make_shared<LeafNode>(parent->key_nibbles, child->value);
        SL_TRACE(logger,
                 "handleDeletion: turn a branch with single leaf child into "
                 "its child");
      } else if (child->isBranch()) {
        branch.children = dynamic_cast<BranchNode &>(*child).children;
        parent->value = child->value;
        SL_TRACE(logger,
                 "handleDeletion: turn a branch with single branch child into "
                 "its child");
      }
      parent->key_nibbles.putUint8(idx).putBuffer(child->key_nibbles);
    }
    return outcome::success();
  }

  /**
   * @return if the node should be deleted itself from parent
   */
  [[nodiscard]] outcome::result<void> deleteNode(
      const kagome::log::Logger &logger,
      PolkadotTrie::NodePtr &node,
      const NibblesView &sought_key,
      OpaqueNodeStorage &node_storage) {
    if (node == nullptr) {
      return outcome::success();
    }
    SL_TRACE(logger,
             "deleteNode: currently in {}, sought key is {}",
             node->key_nibbles.toHex(),
             sought_key);

    if (node->isBranch()) {
      auto &branch = dynamic_cast<BranchNode &>(*node);
      if (node->key_nibbles == sought_key) {
        SL_TRACE(logger, "deleteNode: deleting value in branch; stop");
        node->value = std::nullopt;
      } else {
        auto length = getCommonPrefixLength(node->key_nibbles, sought_key);
        OUTCOME_TRY(child, node_storage.getChild(branch, sought_key[length]));
        SL_TRACE(
            logger, "deleteNode: go to child {:x}", (int)sought_key[length]);
        OUTCOME_TRY(deleteNode(
            logger, child, sought_key.subspan(length + 1), node_storage));
        branch.children[sought_key[length]] = child;
      }
      OUTCOME_TRY(handleDeletion(logger, node, node_storage));
    } else if (node->key_nibbles == sought_key) {
      SL_TRACE(logger, "deleteNode: nullifying leaf node; stop");
      node = nullptr;
    }
    return outcome::success();
  }

  outcome::result<void> notifyOnDetached(
      PolkadotTrie::NodePtr &node,
      const PolkadotTrie::OnDetachCallback &callback) {
    auto key = node->key_nibbles.toByteBuffer();
    OUTCOME_TRY(callback(key, std::move(node->value)));
    return outcome::success();
  }

  /**
   * remove a node with its children
   * @param limit is a max number of values to remove
   * @param finished is true if all values removed, false if limit exceeded
   * @param count is a number of values deleted
   */
  [[nodiscard]] outcome::result<void> detachNode(
      const kagome::log::Logger &logger,
      std::shared_ptr<TrieNode> &parent,
      const KeyNibbles &prefix,
      std::optional<uint64_t> limit,
      bool &finished,
      uint32_t &count,
      const PolkadotTrie::OnDetachCallback &callback,
      OpaqueNodeStorage &node_storage) {
    if (parent == nullptr) {
      return outcome::success();
    }

    // means 'limit ended'
    if (not finished) {
      return outcome::success();
    }

    if (parent->key_nibbles.size() >= prefix.size()) {
      // if this is the node to be detached -- detach it
      if (std::equal(
              prefix.begin(), prefix.end(), parent->key_nibbles.begin())) {
        // remove all children one by one according to limit
        if (parent->isBranch()) {
          auto &branch = dynamic_cast<BranchNode &>(*parent);
          for (uint8_t child_idx = 0; child_idx < branch.kMaxChildren;
               child_idx++) {
            if (branch.children[child_idx] != nullptr) {
              OUTCOME_TRY(child_node, node_storage.getChild(branch, child_idx));
              OUTCOME_TRY(detachNode(logger,
                                     child_node,
                                     KeyNibbles(),
                                     limit,
                                     finished,
                                     count,
                                     callback,
                                     node_storage));
              branch.children[child_idx] = child_node;
            }
          }
        }
        if (not limit or count < limit.value()) {
          if (parent->value) {
            OUTCOME_TRY(notifyOnDetached(parent, callback));
            ++count;
          }
          parent = nullptr;
        } else {
          if (parent->value) {
            // we saw a value after limit, so not finished
            finished = false;
          }
          if (parent->isBranch()) {
            // fix block after children removal
            OUTCOME_TRY(handleDeletion(logger, parent, node_storage));
          }
        }
        return outcome::success();
      }
    }

    // if parent's key is smaller, and it is not a prefix of the prefix, don't
    // change anything
    if (not std::equal(parent->key_nibbles.begin(),
                       parent->key_nibbles.end(),
                       prefix.begin())) {
      return outcome::success();
    }

    if (parent->isBranch()) {
      const auto length = parent->key_nibbles.size();
      auto &branch = dynamic_cast<BranchNode &>(*parent);
      auto &child = branch.children.at(prefix[length]);
      if (child != nullptr) {
        OUTCOME_TRY(child_node, node_storage.getChild(branch, prefix[length]));
        OUTCOME_TRY(detachNode(logger,
                               child_node,
                               prefix.subspan(length + 1),
                               limit,
                               finished,
                               count,
                               callback,
                               node_storage));
        branch.children[prefix[length]] = child_node;
        OUTCOME_TRY(handleDeletion(logger, parent, node_storage));
      }
    }
    return outcome::success();
  }

}  // namespace

namespace kagome::storage::trie {

  PolkadotTrieImpl::PolkadotTrieImpl(NodeRetrieveFunctor f)
      : nodes_{std::make_unique<OpaqueNodeStorage>(std::move(f), nullptr)},
        logger_{log::createLogger("PolkadotTrie", "trie")} {}

  PolkadotTrieImpl::PolkadotTrieImpl(NodePtr root, NodeRetrieveFunctor f)
      : nodes_{std::make_unique<OpaqueNodeStorage>(std::move(f), root)},
        logger_{log::createLogger("PolkadotTrie", "trie")} {}

  PolkadotTrieImpl::~PolkadotTrieImpl() {}

  outcome::result<void> PolkadotTrieImpl::put(const BufferView &key,
                                              const Buffer &value) {
    auto value_copy = value;
    return put(key, std::move(value_copy));
  }

  PolkadotTrie::ConstNodePtr PolkadotTrieImpl::getRoot() const {
    return nodes_->getRoot();
  }

  PolkadotTrie::NodePtr PolkadotTrieImpl::getRoot() {
    return nodes_->getRoot();
  }

  outcome::result<void> PolkadotTrieImpl::put(const BufferView &key,
                                              Buffer &&value) {
    auto k_enc = KeyNibbles::fromByteBuffer(key);

    NodePtr root = nodes_->getRoot();

    // insert fetches a sequence of nodes (a path) from the storage and
    // these nodes are processed in memory, so any changes applied to them
    // will be written back to the storage only on storeNode call
    OUTCOME_TRY(n,
                insert(root, k_enc, std::make_shared<LeafNode>(k_enc, value)));
    nodes_->setRoot(n);

    return outcome::success();
  }

  outcome::result<std::tuple<bool, uint32_t>> PolkadotTrieImpl::clearPrefix(
      const common::BufferView &prefix,
      std::optional<uint64_t> limit,
      const OnDetachCallback &callback) {
    bool finished = true;
    uint32_t count = 0;
    auto key_nibbles = KeyNibbles::fromByteBuffer(prefix);
    auto root = nodes_->getRoot();
    OUTCOME_TRY(detachNode(
        logger_, root, key_nibbles, limit, finished, count, callback, *nodes_));
    nodes_->setRoot(root);
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

    const auto node_type = parent->getTrieType();

    switch (node_type) {
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

        br->key_nibbles = KeyNibbles{key_nibbles.subspan(0, length)};
        auto parentKey = parent->key_nibbles;

        // value goes at this branch
        if (key_nibbles.size() == length) {
          br->value = node->value;

          // if we are not replacing previous leaf, then add it as a
          // child to the new branch
          if (static_cast<std::ptrdiff_t>(parent->key_nibbles.size())
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

      case T::LeafContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::BranchContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::Empty:
        return Error::INVALID_NODE_TYPE;

      case T::ReservedForCompactEncoding:
        return Error::INVALID_NODE_TYPE;

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
    if (not nodes_->getRoot()) {
      return std::nullopt;
    }
    auto nibbles = KeyNibbles::fromByteBuffer(key);
    OUTCOME_TRY(node, getNode(nodes_->getRoot(), nibbles));
    if (node && node->value) {
      return node->value.value();
    }
    return std::nullopt;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::getNode(
      ConstNodePtr parent, const NibblesView &key_nibbles) {
    // SAFETY: changing a parent's opaque child node from a handle to a node
    // to the actual node doesn't break it's const correctness, because opaque
    // nodes are meant to hide their content
    auto const_this = const_cast<const PolkadotTrieImpl *>(this);
    OUTCOME_TRY(const_node, const_this->getNode(parent, key_nibbles));
    // SAFETY: removing constancy we manually added
    auto node = std::const_pointer_cast<TrieNode>(const_node);
    return node;
  }

  outcome::result<PolkadotTrie::ConstNodePtr> PolkadotTrieImpl::getNode(
      ConstNodePtr current, const NibblesView &nibbles) const {
    using T = TrieNode::Type;
    if (current == nullptr) {
      return nullptr;
    }

    const auto node_type = current->getTrieType();
    switch (node_type) {
      case T::BranchEmptyValue:
      case T::BranchWithValue: {
        if (current->key_nibbles == nibbles or nibbles.empty()) {
          return current;
        }
        if (nibbles.size() < static_cast<long>(current->key_nibbles.size())) {
          return nullptr;
        }
        auto parent_as_branch =
            std::dynamic_pointer_cast<const BranchNode>(current);
        auto length = getCommonPrefixLength(current->key_nibbles, nibbles);
        OUTCOME_TRY(n, retrieveChild(*parent_as_branch, nibbles[length]));
        return getNode(n, nibbles.subspan(length + 1));
      }

      case T::Leaf:
        if (current->key_nibbles == nibbles) {
          return current;
        }
        break;

      case T::LeafContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::BranchContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::Empty:
        return Error::INVALID_NODE_TYPE;

      case T::ReservedForCompactEncoding:
        return Error::INVALID_NODE_TYPE;

      default:
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

    const auto node_type = parent->getTrieType();
    switch (node_type) {
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

      case T::LeafContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::BranchContainingHashes:
        return Error::INVALID_NODE_TYPE;

      case T::Empty:
        return Error::INVALID_NODE_TYPE;

      case T::ReservedForCompactEncoding:
        return Error::INVALID_NODE_TYPE;

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
    if (not nodes_->getRoot()) {
      return false;
    }

    OUTCOME_TRY(node,
                getNode(nodes_->getRoot(), KeyNibbles::fromByteBuffer(key)));
    return node != nullptr && node->value;
  }

  bool PolkadotTrieImpl::empty() const {
    return nodes_->getRoot() == nullptr;
  }

  outcome::result<void> PolkadotTrieImpl::remove(
      const common::BufferView &key) {
    auto key_nibbles = KeyNibbles::fromByteBuffer(key);
    // delete node will fetch nodes that it needs from the storage (the
    // nodes typically are a path in the trie) and work on them in memory
    auto root = nodes_->getRoot();
    SL_TRACE(
        logger_, "Remove by key {:l} (nibbles {})", key, key_nibbles.toHex());
    OUTCOME_TRY(deleteNode(logger_, root, key_nibbles, *nodes_));
    nodes_->setRoot(root);
    return outcome::success();
  }

  outcome::result<PolkadotTrie::ConstNodePtr> PolkadotTrieImpl::retrieveChild(
      const BranchNode &parent, uint8_t idx) const {
    OUTCOME_TRY(node, nodes_->getChild(parent, idx));
    return std::move(node);
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      const BranchNode &parent, uint8_t idx) {
    return nodes_->getChild(parent, idx);
  }

}  // namespace kagome::storage::trie

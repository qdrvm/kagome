/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
    OpaqueNodeStorage(PolkadotTrie::NodeRetrieveFunction node_retriever,
                      PolkadotTrie::ValueRetrieveFunction value_retriever,
                      std::shared_ptr<TrieNode> root)
        : retrieve_node_{std::move(node_retriever)},
          retrieve_value_{std::move(value_retriever)},
          root_{std::move(root)} {}

    static outcome::result<std::unique_ptr<OpaqueNodeStorage>> createAt(
        std::shared_ptr<OpaqueTrieNode> root,
        PolkadotTrie::NodeRetrieveFunction node_retriever,
        PolkadotTrie::ValueRetrieveFunction value_retriever) {
      OUTCOME_TRY(root_node, node_retriever(root));
      return std::unique_ptr<OpaqueNodeStorage>{
          new OpaqueNodeStorage{node_retriever, value_retriever, root_node}};
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
        const BranchNode &parent, uint8_t idx) const {
      // SAFETY: changing a parent's opaque child node from a handle to a node
      // to the actual node doesn't break it's const correctness, because opaque
      // nodes are meant to hide their content
      auto &mut_parent = const_cast<BranchNode &>(parent);
      auto &opaque_child = parent.children.at(idx);
      OUTCOME_TRY(child, retrieve_node_(opaque_child));
      mut_parent.children.at(idx) = child;
      return child;
    }

    [[nodiscard]] outcome::result<std::shared_ptr<TrieNode>> getChild(
        const BranchNode &parent, uint8_t idx) {
      // SAFETY: adding constness, not removing
      auto const_this = const_cast<const OpaqueNodeStorage *>(this);
      OUTCOME_TRY(const_child, const_this->getChild(parent, idx));
      // SAFETY: removing constness we manually added
      auto child = std::const_pointer_cast<TrieNode>(const_child);
      return child;
    }

    PolkadotTrie::NodeRetrieveFunction retrieve_node_;
    PolkadotTrie::ValueRetrieveFunction retrieve_value_;
    std::shared_ptr<TrieNode> root_;
  };
}  // namespace kagome::storage::trie

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
  [[nodiscard]] outcome::result<void> handleDeletion(
      const kagome::log::Logger &logger,
      PolkadotTrie::NodePtr &parent,
      OpaqueNodeStorage &node_storage) {
    if (!parent->isBranch()) {
      return outcome::success();
    }
    auto &branch = dynamic_cast<BranchNode &>(*parent);
    auto bitmap = branch.childrenBitmap();
    if (bitmap == 0) {
      if (parent->getValue()) {
        // turn branch node left with no children to a leaf node
        parent = std::make_shared<LeafNode>(parent->getKeyNibbles(),
                                            parent->getValue());
        SL_TRACE(logger, "handleDeletion: turn childless branch into a leaf");
      } else {
        // this case actual only for clearPrefix, unreal situation in deletion
        parent = nullptr;
        SL_TRACE(logger, "handleDeletion: nullify valueless branch parent");
      }
    } else if (branch.childrenNum() == 1 && !branch.getValue()) {
      size_t idx = 0;
      while (bitmap >>= 1u) {
        ++idx;
      }
      OUTCOME_TRY(child, node_storage.getChild(branch, idx));

      if (!child->isBranch()) {
        parent = std::make_shared<LeafNode>(parent->getKeyNibbles(),
                                            child->getValue());
        SL_TRACE(logger,
                 "handleDeletion: turn a branch with single leaf child into "
                 "its child");
      } else {
        branch.children = dynamic_cast<BranchNode &>(*child).children;
        parent->setValue(child->getValue());
        SL_TRACE(logger,
                 "handleDeletion: turn a branch with single branch child into "
                 "its child");
      }
      parent->getMutKeyNibbles().putUint8(idx).put(child->getKeyNibbles());
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
             node->getKeyNibbles().toHex(),
             sought_key);

    if (node->isBranch()) {
      auto &branch = dynamic_cast<BranchNode &>(*node);
      if (node->getKeyNibbles() == sought_key) {
        SL_TRACE(logger, "deleteNode: deleting value in branch; stop");
        node->setValue(ValueAndHash{});
      } else {
        auto length = getCommonPrefixLength(node->getKeyNibbles(), sought_key);
        if (length >= sought_key.size()) {
          return outcome::success();
        }
        OUTCOME_TRY(child, node_storage.getChild(branch, sought_key[length]));
        SL_TRACE(
            logger, "deleteNode: go to child {:x}", (int)sought_key[length]);
        OUTCOME_TRY(deleteNode(
            logger, child, sought_key.subspan(length + 1), node_storage));
        branch.children[sought_key[length]] = child;
      }
      OUTCOME_TRY(handleDeletion(logger, node, node_storage));
    } else if (node->getKeyNibbles() == sought_key) {
      SL_TRACE(logger, "deleteNode: nullifying leaf node; stop");
      node = nullptr;
    }
    return outcome::success();
  }

  outcome::result<void> notifyOnDetached(
      PolkadotTrie::NodePtr &node,
      const PolkadotTrie::OnDetachCallback &callback) {
    auto key = node->getKeyNibbles().toByteBuffer();
    OUTCOME_TRY(callback(key, std::move(node->getMutableValue().value)));
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
      const NibblesView &prefix,
      std::optional<uint64_t> limit,
      bool &finished,
      uint32_t &count,
      const PolkadotTrie::OnDetachCallback &callback,
      PolkadotTrie &trie,
      OpaqueNodeStorage &node_storage) {
    if (parent == nullptr) {
      return outcome::success();
    }

    // means 'limit ended'
    if (not finished) {
      return outcome::success();
    }

    if (std::greater_equal<size_t>()(parent->getKeyNibbles().size(),
                                     prefix.size())) {
      // if this is the node to be detached -- detach it
      if (std::equal(
              prefix.begin(), prefix.end(), parent->getKeyNibbles().begin())) {
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
                                     trie,
                                     node_storage));
              branch.children[child_idx] = child_node;
            }
          }
        }
        if (not limit or count < limit.value()) {
          if (parent->getValue()) {
            OUTCOME_TRY(trie.retrieveValue(parent->getMutableValue()));
            OUTCOME_TRY(notifyOnDetached(parent, callback));
            ++count;
          }
          parent = nullptr;
        } else {
          if (parent->getValue()) {
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
    if (not startsWith(prefix, parent->getKeyNibbles())) {
      return outcome::success();
    }

    if (parent->isBranch()) {
      const auto length = parent->getKeyNibbles().size();
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
                               trie,
                               node_storage));
        branch.children[prefix[length]] = child_node;
        OUTCOME_TRY(handleDeletion(logger, parent, node_storage));
      }
    }
    return outcome::success();
  }

}  // namespace

namespace kagome::storage::trie {
  PolkadotTrieImpl::PolkadotTrieImpl(PolkadotTrieImpl &&) = default;
  PolkadotTrieImpl &PolkadotTrieImpl::operator=(PolkadotTrieImpl &&) = default;

  std::shared_ptr<PolkadotTrieImpl> PolkadotTrieImpl::createEmpty(
      RetrieveFunctions retrieve_functions) {
    return std::shared_ptr<PolkadotTrieImpl>(
        new PolkadotTrieImpl{std::move(retrieve_functions)});
  }

  std::shared_ptr<PolkadotTrieImpl> PolkadotTrieImpl::create(
      NodePtr root, RetrieveFunctions retrieve_functions) {
    return std::shared_ptr<PolkadotTrieImpl>(
        new PolkadotTrieImpl{root, std::move(retrieve_functions)});
  }

  PolkadotTrieImpl::PolkadotTrieImpl(RetrieveFunctions retrieve_functions)
      : nodes_{std::make_unique<OpaqueNodeStorage>(
          std::move(retrieve_functions.retrieve_node),
          std::move(retrieve_functions.retrieve_value),
          nullptr)},
        logger_{log::createLogger("PolkadotTrie", "trie")} {}

  PolkadotTrieImpl::PolkadotTrieImpl(NodePtr root,
                                     RetrieveFunctions retrieve_functions)
      : nodes_{std::make_unique<OpaqueNodeStorage>(
          std::move(retrieve_functions.retrieve_node),
          std::move(retrieve_functions.retrieve_value),
          root)},
        logger_{log::createLogger("PolkadotTrie", "trie")} {}

  PolkadotTrieImpl::~PolkadotTrieImpl() {}

  PolkadotTrie::ConstNodePtr PolkadotTrieImpl::getRoot() const {
    return nodes_->getRoot();
  }

  PolkadotTrie::NodePtr PolkadotTrieImpl::getRoot() {
    return nodes_->getRoot();
  }

  outcome::result<void> PolkadotTrieImpl::put(const BufferView &key,
                                              BufferOrView &&value) {
    auto k_enc = KeyNibbles::fromByteBuffer(key);

    NodePtr root = nodes_->getRoot();

    // insert fetches a sequence of nodes (a path) from the storage and
    // these nodes are processed in memory, so any changes applied to them
    // will be written back to the storage only on storeNode call
    OUTCOME_TRY(n,
                insert(root,
                       k_enc,
                       std::make_shared<LeafNode>(k_enc, value.intoBuffer())));
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
    OUTCOME_TRY(detachNode(logger_,
                           root,
                           key_nibbles,
                           limit,
                           finished,
                           count,
                           callback,
                           *this,
                           *nodes_));
    nodes_->setRoot(root);
    return {finished, count};
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::insert(
      const NodePtr &parent, const NibblesView &key_nibbles, NodePtr node) {
    // just update the node key and return it as the new root
    if (parent == nullptr) {
      node->setKeyNibbles(key_nibbles);
      return node;
    }

    if (parent->isBranch()) {
      auto parent_as_branch = std::dynamic_pointer_cast<BranchNode>(parent);
      return updateBranch(parent_as_branch, key_nibbles, node);
    }
    // need to convert this leaf into a branch
    auto br = std::make_shared<BranchNode>();
    auto length = getCommonPrefixLength(key_nibbles, parent->getKeyNibbles());

    if (parent->getKeyNibbles() == key_nibbles
        && key_nibbles.size() == length) {
      node->setKeyNibbles(key_nibbles);
      return node;
    }

    br->setKeyNibbles(KeyNibbles{key_nibbles.first(length)});
    auto parentKey = parent->getKeyNibbles();

    // value goes at this branch
    if (key_nibbles.size() == length) {
      br->setValue(node->getValue());

      // if we are not replacing previous leaf, then add it as a
      // child to the new branch
      if (parent->getKeyNibbles().size() > key_nibbles.size()) {
        parent->setKeyNibbles(parent->getKeyNibbles().subbuffer(length + 1));
        br->children.at(parentKey[length]) = parent;
      }

      return br;
    }

    node->setKeyNibbles(KeyNibbles{key_nibbles.subspan(length + 1)});

    if (length == parent->getKeyNibbles().size()) {
      // if leaf's key is covered by this branch, then make the leaf's
      // value the value at this branch
      br->setValue(parent->getValue());
      br->children.at(key_nibbles[length]) = node;
    } else {
      // otherwise, make the leaf a child of the branch and update its
      // partial key
      parent->setKeyNibbles(parent->getKeyNibbles().subbuffer(length + 1));
      br->children.at(parentKey[length]) = parent;
      br->children.at(key_nibbles[length]) = node;
    }

    return br;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::updateBranch(
      BranchPtr parent, const NibblesView &key_nibbles, const NodePtr &node) {
    auto length = getCommonPrefixLength(key_nibbles, parent->getKeyNibbles());

    if (length == parent->getKeyNibbles().size()) {
      // just set the value in the parent to the node value
      if (key_nibbles == parent->getKeyNibbles()) {
        parent->setValue(node->getValue());
        return parent;
      }
      OUTCOME_TRY(child, retrieveChild(*parent, key_nibbles[length]));
      if (child) {
        OUTCOME_TRY(n, insert(child, key_nibbles.subspan(length + 1), node));
        parent->children.at(key_nibbles[length]) = n;
        return parent;
      }
      node->setKeyNibbles(KeyNibbles{key_nibbles.subspan(length + 1)});
      parent->children.at(key_nibbles[length]) = node;
      return parent;
    }
    auto br =
        std::make_shared<BranchNode>(KeyNibbles{key_nibbles.first(length)});
    auto parentIdx = parent->getKeyNibbles()[length];
    OUTCOME_TRY(
        new_branch,
        insert(nullptr, parent->getKeyNibbles().subspan(length + 1), parent));
    br->children.at(parentIdx) = new_branch;
    if (key_nibbles.size() <= length) {
      br->setValue(node->getValue());
    } else {
      OUTCOME_TRY(new_child,
                  insert(nullptr, key_nibbles.subspan(length + 1), node));
      br->children.at(key_nibbles[length]) = new_child;
    }
    return br;
  }

  outcome::result<BufferOrView> PolkadotTrieImpl::get(
      const common::BufferView &key) const {
    OUTCOME_TRY(opt_value, tryGet(key));
    if (opt_value.has_value()) {
      return std::move(*opt_value);
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<std::optional<BufferOrView>> PolkadotTrieImpl::tryGet(
      const common::BufferView &key) const {
    if (not nodes_->getRoot()) {
      return std::nullopt;
    }
    auto nibbles = KeyNibbles::fromByteBuffer(key);
    OUTCOME_TRY(node, getNode(nodes_->getRoot(), nibbles));
    if (node && node->getValue()) {
      OUTCOME_TRY(retrieveValue(const_cast<ValueAndHash &>(node->getValue())));
      return BufferView{*node->getValue().value};
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
    if (current == nullptr) {
      return nullptr;
    }

    if (current->isBranch()) {
      if (current->getKeyNibbles() == nibbles or nibbles.empty()) {
        return current;
      }
      if (nibbles.size() < current->getKeyNibbles().size()) {
        return nullptr;
      }
      auto parent_as_branch =
          std::dynamic_pointer_cast<const BranchNode>(current);
      auto length = getCommonPrefixLength(current->getKeyNibbles(), nibbles);
      OUTCOME_TRY(n, retrieveChild(*parent_as_branch, nibbles[length]));
      return getNode(n, nibbles.subspan(length + 1));
    }
    if (current->getKeyNibbles() == nibbles) {
      return current;
    }
    return nullptr;
  }

  outcome::result<void> PolkadotTrieImpl::forNodeInPath(
      ConstNodePtr parent,
      const NibblesView &path,
      const BranchVisitor &callback) const {
    if (parent == nullptr) {
      return TrieError::NO_VALUE;
    }

    if (parent->isBranch()) {
      // path is completely covered by the parent key
      if (parent->getKeyNibbles() == path or path.empty()) {
        return outcome::success();
      }
      auto common_length = getCommonPrefixLength(parent->getKeyNibbles(), path);
      auto common_nibbles = parent->getKeyNibbles().view(0, common_length);
      // path is even less than the parent key (path is the prefix of the
      // parent key)
      if (path == common_nibbles
          and path.size() < parent->getKeyNibbles().size()) {
        return outcome::success();
      }
      auto parent_as_branch =
          std::dynamic_pointer_cast<const BranchNode>(parent);
      OUTCOME_TRY(child, retrieveChild(*parent_as_branch, path[common_length]));
      OUTCOME_TRY(callback(*parent_as_branch, path[common_length], *child));
      return forNodeInPath(child, path.subspan(common_length + 1), callback);
    }
    if (parent->getKeyNibbles() == path) {
      return outcome::success();
    }
    return TrieError::NO_VALUE;
  }

  std::unique_ptr<PolkadotTrieCursor> PolkadotTrieImpl::trieCursor() const {
    return std::make_unique<PolkadotTrieCursorImpl>(shared_from_this());
  }

  outcome::result<bool> PolkadotTrieImpl::contains(
      const common::BufferView &key) const {
    if (not nodes_->getRoot()) {
      return false;
    }

    OUTCOME_TRY(node,
                getNode(nodes_->getRoot(), KeyNibbles::fromByteBuffer(key)));
    return node != nullptr && node->getValue();
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
    return node;
  }

  outcome::result<PolkadotTrie::NodePtr> PolkadotTrieImpl::retrieveChild(
      const BranchNode &parent, uint8_t idx) {
    return nodes_->getChild(parent, idx);
  }

  outcome::result<void> PolkadotTrieImpl::retrieveValue(
      ValueAndHash &value) const {
    if (value.hash && !value.value) {
      OUTCOME_TRY(loaded_value, nodes_->retrieve_value_(*value.hash));
      value.value = std::move(loaded_value);
      if (!value.value) {
        return TrieError::BROKEN_VALUE;
      }
    }
    return outcome::success();
  }
}  // namespace kagome::storage::trie

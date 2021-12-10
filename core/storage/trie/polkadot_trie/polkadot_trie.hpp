/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP
#define KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP

#include "storage/buffer_map_types.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  /**
   * For specification see Polkadot Runtime Environment Protocol Specification
   * '2.1.2 The General Tree Structure' and further
   */
  class PolkadotTrie
      : public face::
            ReadOnlyMap<common::Buffer, common::Buffer, common::BufferView>,
        public face::Writeable<common::BufferView, common::Buffer> {
   public:
    using NodePtr = std::shared_ptr<TrieNode>;
    using ConstNodePtr = std::shared_ptr<const TrieNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;
    using ConstBranchPtr = std::shared_ptr<const BranchNode>;
    using NodeRetrieveFunctor =
        std::function<outcome::result<NodePtr>(const NodePtr &)>;

    /**
     * This callback is called when a node is detached from a trie. It is called
     * for each leaf from the detached node subtree
     */
    using OnDetachCallback = std::function<outcome::result<void>(
        const common::BufferView &key, std::optional<common::Buffer> &&value)>;

    /**
     * Remove all trie entries which key begins with the supplied prefix
     * @param prefix key prefix for nodes to be removed
     * @param limit number of elements to remove, std::nullopt if no limit
     * @param callback function that will be called for each node removal
     * @returns tuple first element true if removed all nodes to be removed,
     * second tuple element is a number of removed elements
     */

    virtual outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const common::BufferView &prefix,
        std::optional<uint64_t> limit,
        const OnDetachCallback &callback) = 0;

    /**
     * @return the root node of the trie
     */
    virtual NodePtr getRoot() = 0;
    virtual ConstNodePtr getRoot() const = 0;

    /**
     * @returns a child node pointer of a provided \arg parent node
     * at the index \idx
     */
    virtual outcome::result<ConstNodePtr> retrieveChild(const BranchNode &parent,
                                                        uint8_t idx) const = 0;
    virtual outcome::result<NodePtr> retrieveChild(const BranchNode &parent,
                                                   uint8_t idx) = 0;

    /**
     * @returns a node which is a descendant of \arg parent found by following
     * \arg key_nibbles (includes parent's key nibbles)
     */
    virtual outcome::result<NodePtr> getNode(ConstNodePtr parent,
                                             const KeyNibbles &key_nibbles) = 0;
    virtual outcome::result<ConstNodePtr> getNode(
        ConstNodePtr parent, const KeyNibbles &key_nibbles) const = 0;

    /**
     * Invokes callback on each node starting from \arg parent and ending on the
     * node corresponding to the \arg path
     * @returns error, if any call errored, success otherwise
     */
    virtual outcome::result<void> forNodeInPath(
        ConstNodePtr parent,
        const KeyNibbles &path,
        const std::function<outcome::result<void>(
            BranchNode const &, uint8_t idx)> &callback) const = 0;

    virtual std::unique_ptr<PolkadotTrieCursor> trieCursor() = 0;

    std::unique_ptr<Cursor> cursor() final {
      return trieCursor();
    }

    inline static outcome::result<NodePtr> defaultNodeRetrieveFunctor(
        const NodePtr &node) {
      BOOST_ASSERT_MSG(not node or not node->isDummy(),
                       "Dummy node unexpected.");
      return node;
    }
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP

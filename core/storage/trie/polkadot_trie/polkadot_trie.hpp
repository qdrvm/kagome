/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP
#define KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP

#include "storage/buffer_map_types.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  /**
   * For specification see Polkadot Runtime Environment Protocol Specification
   * '2.1.2 The General Tree Structure' and further
   */
  class PolkadotTrie : public BufferStorage,
                       public std::enable_shared_from_this<PolkadotTrie> {
   public:
    using NodePtr = std::shared_ptr<TrieNode>;
    using ConstNodePtr = std::shared_ptr<const TrieNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;
    using ConstBranchPtr = std::shared_ptr<const BranchNode>;

    using NodeRetrieveFunction = std::function<outcome::result<NodePtr>(
        const std::shared_ptr<OpaqueTrieNode> &)>;
    using ValueRetrieveFunction =
        std::function<outcome::result<std::optional<common::Buffer>>(
            const common::Hash256 & /* value hash */)>;

    struct RetrieveFunctions {
      RetrieveFunctions()
          : retrieve_node{defaultNodeRetrieve},
            retrieve_value{defaultValueRetrieve} {}

      RetrieveFunctions(NodeRetrieveFunction retrieve_node,
                        ValueRetrieveFunction retrieve_value)
          : retrieve_node{std::move(retrieve_node)},
            retrieve_value{std::move(retrieve_value)} {}

      inline static outcome::result<NodePtr> defaultNodeRetrieve(
          const std::shared_ptr<OpaqueTrieNode> &node) {
        BOOST_ASSERT_MSG(
            node == nullptr
                or std::dynamic_pointer_cast<TrieNode>(node) != nullptr,
            "Unexpected Dummy node.");
        return std::dynamic_pointer_cast<TrieNode>(node);
      }

      inline static outcome::result<std::optional<common::Buffer>>
      defaultValueRetrieve(const common::Hash256 &) {
        return TrieError::VALUE_RETRIEVE_NOT_PROVIDED;
      }

      NodeRetrieveFunction retrieve_node;
      ValueRetrieveFunction retrieve_value;
    };

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
    virtual outcome::result<ConstNodePtr> retrieveChild(
        const BranchNode &parent, uint8_t idx) const = 0;
    virtual outcome::result<NodePtr> retrieveChild(const BranchNode &parent,
                                                   uint8_t idx) = 0;

    /**
     * Retrieve value from hash if value is not present.
     */
    virtual outcome::result<void> retrieveValue(ValueAndHash &value) const = 0;

    /**
     * @returns a node which is a descendant of \arg parent found by following
     * \arg key_nibbles (includes parent's key nibbles)
     */
    virtual outcome::result<NodePtr> getNode(
        ConstNodePtr parent, const NibblesView &key_nibbles) = 0;
    virtual outcome::result<ConstNodePtr> getNode(
        ConstNodePtr parent, const NibblesView &key_nibbles) const = 0;

    using BranchVisitor = std::function<outcome::result<void>(
        const BranchNode &, uint8_t idx, const TrieNode &child)>;
    /**
     * Invokes callback on each node starting from \arg parent and ending on the
     * node corresponding to the \arg path
     * @returns error, if any call errored, success otherwise
     */
    virtual outcome::result<void> forNodeInPath(
        ConstNodePtr parent,
        const NibblesView &path,
        const BranchVisitor &callback) const = 0;

    virtual std::unique_ptr<PolkadotTrieCursor> trieCursor() const = 0;

    std::unique_ptr<Cursor> cursor() final {
      return trieCursor();
    }
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_TRIE_HPP

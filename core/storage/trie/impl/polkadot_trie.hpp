/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_POLKADOT_TRIE_HPP
#define KAGOME_POLKADOT_TRIE_HPP

#include "storage/face/generic_maps.hpp"
#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_node.hpp"

namespace kagome::storage::trie {

  /**
   * For specification see Polkadot Runtime Environment Protocol Specification
   * '2.1.2 The General Tree Structure' and further
   */
  class PolkadotTrie : public face::GenericMap<common::Buffer, common::Buffer> {
    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;
    using ChildRetrieveFunctor =
        std::function<outcome::result<NodePtr>(BranchPtr, uint8_t)>;

    friend class PolkadotTrieCursor;

    // a child is obtained from the branch list of children as-is.
    // should be used when the trie is completely in memory
    inline static outcome::result<NodePtr> defaultChildRetrieveFunctor(
        const BranchPtr &parent, uint8_t idx) {
      return parent->getChild(idx);
    }

   public:
    enum class Error { INVALID_NODE_TYPE = 1 };

    /**
     * Creates an empty Trie
     * @param f a functor that will be used to obtain a child of a branch node
     * by its index. Most useful if Trie grows too big to occupy main memory and
     * is stored on an external storage
     */
    explicit PolkadotTrie(ChildRetrieveFunctor f = defaultChildRetrieveFunctor);

    explicit PolkadotTrie(NodePtr root,
                          ChildRetrieveFunctor f = defaultChildRetrieveFunctor);

    /**
     * @return the root node of the trie
     */
    NodePtr getRoot();

    /**
     * Remove all entries, which key starts with the prefix
     */
    outcome::result<void> clearPrefix(const common::Buffer &prefix);

    // value will be copied
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;

    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;

    bool contains(const common::Buffer &key) const override;

    auto cursor() -> std::unique_ptr<
        face::MapCursor<common::Buffer, common::Buffer>> override;

   private:
    outcome::result<NodePtr> insert(const NodePtr &parent,
                                    const common::Buffer &key_nibbles,
                                    NodePtr node);

    outcome::result<NodePtr> updateBranch(BranchPtr parent,
                                          const common::Buffer &key_nibbles,
                                          const NodePtr &node);

    outcome::result<NodePtr> deleteNode(NodePtr parent,
                                        const common::Buffer &key_nibbles);
    outcome::result<NodePtr> handleDeletion(const BranchPtr &parent,
                                            NodePtr node,
                                            const common::Buffer &key_nibbles);
    // remove a node with its children
    outcome::result<NodePtr> detachNode(const NodePtr &parent,
                                        const common::Buffer &prefix_nibbles);
    outcome::result<NodePtr> getNode(NodePtr parent,
                                     const common::Buffer &key_nibbles) const;

    uint32_t getCommonPrefixLength(const common::Buffer &pref1,
                                   const common::Buffer &pref2) const;

    outcome::result<NodePtr> retrieveChild(BranchPtr parent, uint8_t idx) const;

    ChildRetrieveFunctor retrieve_child_;
    NodePtr root_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrie::Error);

#endif  // KAGOME_POLKADOT_TRIE_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

#include "storage/buffer_map_types.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieImpl : public PolkadotTrie {
    // a child is obtained from the branch list of children as-is.
    // should be used when the trie is completely in memory
    inline static outcome::result<NodePtr> defaultChildRetrieveFunctor(
        const BranchPtr &parent, uint8_t idx) {
      return parent->children.at(idx);
    }

   public:
    using ChildRetrieveFunctor =
        std::function<outcome::result<NodePtr>(BranchPtr, uint8_t)>;

    enum class Error { INVALID_NODE_TYPE = 1 };

    /**
     * Creates an empty Trie
     * @param f a functor that will be used to obtain a child of a branch node
     * by its index. Most useful if Trie grows too big to occupy main memory and
     * is stored on an external storage
     */
    explicit PolkadotTrieImpl(
        ChildRetrieveFunctor f = defaultChildRetrieveFunctor);

    explicit PolkadotTrieImpl(
        NodePtr root, ChildRetrieveFunctor f = defaultChildRetrieveFunctor);

    NodePtr getRoot() const override;

    /**
     * Remove all entries, which key starts with the prefix
     */
    outcome::result<void> clearPrefix(const common::Buffer &prefix) override;

    // value will be copied
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;

    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;

    std::unique_ptr<BufferMapCursor> cursor() override;

    bool contains(const common::Buffer &key) const override;

    bool empty() const override;

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
    outcome::result<NodePtr> getNode(
        NodePtr parent, const common::Buffer &key_nibbles) const override;

    uint32_t getCommonPrefixLength(const common::Buffer &pref1,
                                   const common::Buffer &pref2) const;

    outcome::result<NodePtr> retrieveChild(BranchPtr parent,
                                           uint8_t idx) const override;

    ChildRetrieveFunctor retrieve_child_;
    NodePtr root_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieImpl::Error);

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL

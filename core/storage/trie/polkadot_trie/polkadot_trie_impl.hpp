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
   public:
    enum class Error { INVALID_NODE_TYPE = 1 };

    /**
     * Creates an empty Trie
     * @param f a functor that will be used to obtain a child of a branch node
     * by its index. Most useful if Trie grows too big to occupy main memory and
     * is stored on an external storage
     */
    explicit PolkadotTrieImpl(PolkadotTrie::NodeRetrieveFunctor f =
                                  PolkadotTrie::defaultNodeRetrieveFunctor);

    explicit PolkadotTrieImpl(NodePtr root,
                              PolkadotTrie::NodeRetrieveFunctor f =
                                  PolkadotTrie::defaultNodeRetrieveFunctor);

    NodePtr getRoot() const override;

    outcome::result<NodePtr> getNode(
        NodePtr parent, const KeyNibbles &key_nibbles) const override;

    outcome::result<std::list<std::pair<BranchPtr, uint8_t>>> getPath(
        NodePtr parent, const KeyNibbles &key_nibbles) const override;

    /**
     * Remove all entries, which key starts with the prefix
     */
    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const common::Buffer &prefix,
        std::optional<uint64_t> limit,
        const OnDetachCallback &callback) override;

    // value will be copied
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;

    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;

    outcome::result<std::optional<common::Buffer>> tryGet(
        const common::Buffer &key) const override;

    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;

    bool contains(const common::Buffer &key) const override;

    bool empty() const override;

   private:
    outcome::result<NodePtr> insert(const NodePtr &parent,
                                    const KeyNibbles &key_nibbles,
                                    NodePtr node);

    outcome::result<NodePtr> updateBranch(BranchPtr parent,
                                          const KeyNibbles &key_nibbles,
                                          const NodePtr &node);

    outcome::result<NodePtr> retrieveChild(BranchPtr parent,
                                           uint8_t idx) const override;

    NodeRetrieveFunctor retrieve_node_;
    NodePtr root_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieImpl::Error);

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL

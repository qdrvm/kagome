/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL
#define KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {

  class OpaqueNodeStorage;

  class PolkadotTrieImpl final
      : public PolkadotTrie,
        public std::enable_shared_from_this<PolkadotTrieImpl> {
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
    ~PolkadotTrieImpl();

    NodePtr getRoot() override;
    ConstNodePtr getRoot() const override;

    outcome::result<NodePtr> getNode(ConstNodePtr parent,
                                     const NibblesView &key_nibbles) override;
    outcome::result<ConstNodePtr> getNode(
        ConstNodePtr parent, const NibblesView &key_nibbles) const override;

    outcome::result<void> forNodeInPath(
        ConstNodePtr parent,
        const NibblesView &path,
        const std::function<outcome::result<void>(
            BranchNode const &, uint8_t idx)> &callback) const override;

    /**
     * Remove all entries, which key starts with the prefix
     */
    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const common::BufferView &prefix,
        std::optional<uint64_t> limit,
        const OnDetachCallback &callback) override;

    // value will be copied
    outcome::result<void> put(const common::BufferView &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::BufferView &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::BufferView &key) override;

    outcome::result<common::BufferConstRef> get(
        const common::BufferView &key) const override;

    outcome::result<std::optional<common::BufferConstRef>> tryGet(
        const common::BufferView &key) const override;

    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;

    outcome::result<bool> contains(
        const common::BufferView &key) const override;

    bool empty() const override;

    outcome::result<ConstNodePtr> retrieveChild(
        const BranchNode &parent, uint8_t idx) const override;
    outcome::result<NodePtr> retrieveChild(const BranchNode &parent,
                                                   uint8_t idx) override;

   private:
    outcome::result<NodePtr> insert(const NodePtr &parent,
                                    const NibblesView &key_nibbles,
                                    NodePtr node);

    outcome::result<NodePtr> updateBranch(BranchPtr parent,
                                          const NibblesView &key_nibbles,
                                          const NodePtr &node);

    std::unique_ptr<OpaqueNodeStorage> nodes_;

    log::Logger logger_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotTrieImpl::Error);

#endif  // KAGOME_STORAGE_TRIE_IMPL_POLKADOT_TRIE_IMPL

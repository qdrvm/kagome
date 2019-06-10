/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP_
#define KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP_

#include <map>
#include <memory>
#include <optional>

#include "crypto/hasher.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_codec.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"
#include "storage/merkle/trie_db.hpp"

namespace kagome::storage::merkle {

  /**
   * For specification see https://github.com/w3f/polkadot-re-spec/blob/master/polkadot_re_spec.pdf
   * 5.2 The General Tree Structure and further
   */
  class PolkadotTrieDb : public TrieDb {
    using MapCursor = face::MapCursor<common::Buffer, common::Buffer>;
    using WriteBatch = face::WriteBatch<common::Buffer, common::Buffer>;
    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;

    // to output the trie into a stream
    template <typename Stream>
    friend Stream &operator<<(Stream &s, const PolkadotTrieDb &trie);
    template <typename Stream>
    friend Stream &printNode(Stream &s, NodePtr node,
                             const PolkadotTrieDb &trie, size_t nest_level);

   public:
    enum class Error { INVALID_NODE_TYPE = 1 };

   public:
    explicit PolkadotTrieDb(std::unique_ptr<PersistentBufferMap> db = );
    ~PolkadotTrieDb() override = default;

    common::Buffer getRootHash() const override;
    outcome::result<void> clearPrefix(const common::Buffer &prefix) override;

    std::unique_ptr<WriteBatch> batch() override;

    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;
    outcome::result<void> remove(const common::Buffer &key) override;
    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;
    bool contains(const common::Buffer &key) const override;

    std::unique_ptr<MapCursor> cursor() override;

   private:
    outcome::result<void> insertRoot(const common::Buffer &key_nibbles,
                                     const common::Buffer &value);

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

    /**
     * Writes a node to a persistent storage, recursively storing its
     * descendants as well. Then replaces the node children to dummy nodes to
     * avoid memory waste
     */
    outcome::result<common::Buffer> storeNode(PolkadotNode &node);
    outcome::result<common::Buffer> storeNode(PolkadotNode &node,
                                              WriteBatch &batch);
    /**
     * Fetches a node from the storage. A nullptr is returned in case that there
     * is no entry for provided key. Mind that a branch node will have dummy
     * nodes as its children
     */
    outcome::result<NodePtr> retrieveNode(const common::Buffer &db_key) const;
    /**
     * Retrieves a node child, replacing a dummy node to an actual node if
     * needed
     */
    outcome::result<NodePtr> retrieveChild(const BranchPtr &parent,
                                           uint8_t idx) const;

    std::unique_ptr<PersistentBufferMap> db_;
    std::optional<common::Buffer> root_;
    PolkadotCodec codec_;
  };

}  // namespace kagome::storage::merkle

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::merkle, PolkadotTrieDb::Error);

#endif  // KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP_

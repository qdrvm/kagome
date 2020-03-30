/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP

#include <memory>
#include <optional>

#include "crypto/hasher.hpp"
#include "storage/trie/impl/polkadot_codec.hpp"
#include "storage/trie/impl/polkadot_node.hpp"
#include "storage/trie/impl/polkadot_trie.hpp"
#include "storage/trie/trie_db.hpp"
#include "storage/trie/trie_db_backend.hpp"

namespace kagome::storage::trie {

  /**
   * A wrapper for PolkadotTrie that allows storing the trie in an external
   * storage that supports PersistentBufferMap interface
   */
  class PolkadotTrieDb : public TrieDb {
    using MapCursor = face::MapCursor<common::Buffer, common::Buffer>;
    using WriteBatch = face::WriteBatch<common::Buffer, common::Buffer>;
    using NodePtr = std::shared_ptr<PolkadotNode>;
    using BranchPtr = std::shared_ptr<BranchNode>;

    friend class PolkadotTrieBatch;

    // to output the trie into a stream
    template <typename Stream>
    friend Stream &operator<<(Stream &s, const PolkadotTrieDb &trie);
    template <typename Stream>
    friend Stream &printNode(Stream &s,
                             const NodePtr &node,
                             const PolkadotTrieDb &trie,
                             size_t nest_level);

   public:
    /**
     * Initializes the trie from the provided storage (and will use the storage
     * further)
     */
    static std::unique_ptr<PolkadotTrieDb> createFromStorage(
        common::Buffer root, std::shared_ptr<TrieDbBackend> backend);

    /**
     * Creates an empty trie on the provided storage
     */
    static std::unique_ptr<PolkadotTrieDb> createEmpty(
        std::shared_ptr<TrieDbBackend> backend);

    /**
     * Initializes the trie from the provided storage in read-only mode
     * Mostly required to restore the trie state at a specific moment in time on
     * the blockchain
     */
    static std::unique_ptr<TrieDbReader> initReadOnlyFromStorage(
        common::Buffer root, std::shared_ptr<TrieDbBackend> backend);

    ~PolkadotTrieDb() override = default;

    common::Buffer getRootHash() const override;

    outcome::result<void> clearPrefix(const common::Buffer &prefix) override;

    std::unique_ptr<WriteBatch> batch() override;

    // value will be copied
    outcome::result<void> put(const common::Buffer &key,
                              const common::Buffer &value) override;

    outcome::result<void> put(const common::Buffer &key,
                              common::Buffer &&value) override;

    outcome::result<void> remove(const common::Buffer &key) override;

    outcome::result<common::Buffer> get(
        const common::Buffer &key) const override;

    bool contains(const common::Buffer &key) const override;

    /**
     * @return the root hash of empty Trie
     */
    common::Buffer getEmptyRoot() const;

    bool empty() const override;

    std::unique_ptr<MapCursor> cursor() override;

   protected:
    PolkadotTrieDb(std::shared_ptr<TrieDbBackend> db,
                   boost::optional<common::Buffer> root_hash);

    /**
     * Creates an in-memory trie, which will fetch from the storage only the
     * nodes that are required to complete operations applied to the trie.
     * Usually it's the path from the root to the place of insertion/deletion
     */
    outcome::result<PolkadotTrie> initTrie() const;

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

   private:
    std::shared_ptr<TrieDbBackend> db_;
    PolkadotCodec codec_;
    common::Buffer root_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_HPP

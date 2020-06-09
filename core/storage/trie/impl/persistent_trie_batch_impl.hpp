/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH

#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class PersistentTrieBatchImpl : public PersistentTrieBatch {
   public:
    using RootChangedEventHandler = std::function<void(const common::Buffer&)>;

    PersistentTrieBatchImpl(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
        std::unique_ptr<PolkadotTrie> trie,
        RootChangedEventHandler handler);
    ~PersistentTrieBatchImpl() override = default;

    /**
     * Commits changes to persistent storage
     */
    outcome::result<Buffer> commit() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;
    bool empty() const override;
    outcome::result<Buffer> calculateRoot() const override;
    outcome::result<void> clearPrefix(const Buffer &prefix) override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes_;
    std::unique_ptr<PolkadotTrie> trie_;
    RootChangedEventHandler root_changed_handler_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH

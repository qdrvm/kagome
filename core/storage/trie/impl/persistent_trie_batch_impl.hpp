/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH

#include <memory>

#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class PersistentTrieBatchImpl final : public PersistentTrieBatch {
   public:
    using RootChangedEventHandler = std::function<void(const RootHash &)>;
    enum class Error {
      NO_TRIE = 1,
    };

    static std::unique_ptr<PersistentTrieBatchImpl> create(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
        std::shared_ptr<PolkadotTrie> trie,
        RootChangedEventHandler &&handler);
    ~PersistentTrieBatchImpl() override = default;

    outcome::result<RootHash> commit() override;
    std::unique_ptr<TopperTrieBatch> batchOnTop() override;

    outcome::result<Buffer> get(const Buffer &key) const override;
    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    bool contains(const Buffer &key) const override;
    bool empty() const override;
    outcome::result<void> clearPrefix(const Buffer &prefix) override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    PersistentTrieBatchImpl(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
        std::shared_ptr<PolkadotTrie> trie,
        RootChangedEventHandler &&handler);

    void init();

    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes_;
    std::shared_ptr<PolkadotTrie> trie_;
    RootChangedEventHandler root_changed_handler_;

    log::Logger logger_ =
        log::createLogger("PersistentTrieBatch", "storage");
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie,
                          PersistentTrieBatchImpl::Error);

#endif  // KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH

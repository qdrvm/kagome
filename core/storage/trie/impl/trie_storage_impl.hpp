/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL

#include "storage/trie/trie_storage.hpp"

#include "common/logger.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class Session;
}

namespace kagome::storage::trie {

  class TrieStorageImpl : public TrieStorage {
   public:
    static outcome::result<std::unique_ptr<TrieStorageImpl>> createEmpty(
        const std::shared_ptr<PolkadotTrieFactory> &trie_factory,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

    static outcome::result<std::unique_ptr<TrieStorageImpl>> createFromStorage(
        const common::Buffer &root_hash,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

    TrieStorageImpl(TrieStorageImpl const &) = delete;
    void operator=(const TrieStorageImpl &) = delete;

    TrieStorageImpl(TrieStorageImpl &&) = default;
    TrieStorageImpl &operator=(TrieStorageImpl &&) = default;
    ~TrieStorageImpl() override = default;

    outcome::result<std::unique_ptr<PersistentTrieBatch>> getPersistentBatch()
        override;
    outcome::result<std::unique_ptr<EphemeralTrieBatch>> getEphemeralBatch()
        const override;

    outcome::result<std::unique_ptr<PersistentTrieBatch>> getPersistentBatchAt(
        const common::Hash256 &root) override;
    outcome::result<std::unique_ptr<EphemeralTrieBatch>> getEphemeralBatchAt(
        const common::Hash256 &root) const override;

    common::Buffer getRootHash() const override;

   protected:
    TrieStorageImpl(
        common::Buffer root_hash,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

   private:
    common::Buffer root_hash_;
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes_;
    common::Logger logger_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL

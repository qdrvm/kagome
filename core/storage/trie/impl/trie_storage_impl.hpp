/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL

#include "storage/trie/trie_storage.hpp"

#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {

  class TrieStorageImpl : public TrieStorage {
   public:
    using EncodedNode = common::BufferView;
    using OnNodeLoaded = std::function<void(EncodedNode)>;

    static outcome::result<std::unique_ptr<TrieStorageImpl>> createEmpty(
        const std::shared_ptr<PolkadotTrieFactory> &trie_factory,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer);

    static outcome::result<std::unique_ptr<TrieStorageImpl>> createFromStorage(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer);

    TrieStorageImpl(TrieStorageImpl const &) = delete;
    void operator=(const TrieStorageImpl &) = delete;

    TrieStorageImpl(TrieStorageImpl &&) = default;
    TrieStorageImpl &operator=(TrieStorageImpl &&) = default;
    ~TrieStorageImpl() override = default;

    outcome::result<std::unique_ptr<TrieBatch>> getPersistentBatchAt(
        const RootHash &root, TrieChangesTrackerOpt changes_tracker) override;
    outcome::result<std::unique_ptr<TrieBatch>> getEphemeralBatchAt(
        const RootHash &root) const override;
    outcome::result<std::unique_ptr<TrieBatch>> getProofReaderBatchAt(
        const RootHash &root,
        const OnNodeLoaded &on_node_loaded) const override;

   protected:
    TrieStorageImpl(std::shared_ptr<Codec> codec,
                    std::shared_ptr<TrieSerializer> serializer);

   private:
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    log::Logger logger_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL

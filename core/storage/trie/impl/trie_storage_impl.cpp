/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/trie_storage_impl.hpp"

#include <memory>

#include "outcome/outcome.hpp"
#include "storage/trie/impl/ephemeral_trie_batch_impl.hpp"
#include "storage/trie/impl/persistent_trie_batch_impl.hpp"

namespace kagome::storage::trie {

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createEmpty(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<changes_trie::ChangesTracker> changes) {
    auto empty_root = serializer->getEmptyRootHash();
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(empty_root),
                            std::move(codec),
                            std::move(serializer),
                            std::move(changes)),
                            );
  }

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createFromStorage(
      const common::Buffer &root_hash,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<changes_trie::ChangesTracker> changes,
      std::shared_ptr<PolkadotTrieFactory> trie_factory) {
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(root_hash,
                            std::move(codec),
                            std::move(serializer),
                            std::move(changes)));
  }

  TrieStorageImpl::TrieStorageImpl(
      common::Buffer root_hash,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<changes_trie::ChangesTracker> changes,
      std::shared_ptr<PolkadotTrieFactory> trie_factory)
      : root_hash_{std::move(root_hash)},
        codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        changes_{std::move(changes)},
        trie_factory_{std::move(trie_factory)} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(changes_ != nullptr);
    BOOST_ASSERT(trie_factory_ != nullptr);
  }

  outcome::result<std::unique_ptr<PersistentTrieBatch>>
  TrieStorageImpl::getPersistentBatch() {
    std::unique_ptr<PolkadotTrie> trie;
    if (root_hash_ == serializer_->getEmptyRootHash())

      OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return std::make_unique<PersistentTrieBatchImpl>(
        codec_,
        serializer_,
        changes_,
        std::shared_ptr(std::move(trie)),
        [this](auto const &new_root) { root_hash_ = new_root; });
  }

  outcome::result<std::unique_ptr<EphemeralTrieBatch>>
  TrieStorageImpl::getEphemeralBatch() const {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::shared_ptr(std::move(trie)));
  }

  common::Buffer TrieStorageImpl::getRootHash() const {
    return root_hash_;
  }
}  // namespace kagome::storage::trie

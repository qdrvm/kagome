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
      const std::shared_ptr<PolkadotTrieFactory> &trie_factory,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes) {
    // will never be used, so content of the callback doesn't matter
    auto empty_trie =
        trie_factory->createEmpty([](auto &) { return outcome::success(); });
    // ensure retrieval of empty trie succeeds
    OUTCOME_TRY(_, serializer->storeTrie(*empty_trie));
    return std::unique_ptr<TrieStorageImpl>(new TrieStorageImpl(
            std::move(codec), std::move(serializer), std::move(changes)));
  }

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createFromStorage(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes) {
    return std::unique_ptr<TrieStorageImpl>(new TrieStorageImpl(
        std::move(codec), std::move(serializer), std::move(changes)));
  }

  TrieStorageImpl::TrieStorageImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        changes_{std::move(changes)},
        logger_{log::createLogger("TrieStorage", "storage")} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT((changes_.has_value() and changes_.value() != nullptr)
                 or not changes_.has_value());
    logger_->verbose("Initialize trie storage");
  }

  outcome::result<std::unique_ptr<PersistentTrieBatch>>
  TrieStorageImpl::getPersistentBatchAt(const RootHash &root) {
    SL_DEBUG(logger_,
             "Initialize persistent trie batch with root: {}",
             root.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}));
    return PersistentTrieBatchImpl::create(
        codec_, serializer_, changes_, std::move(trie));
  }

  outcome::result<std::unique_ptr<EphemeralTrieBatch>>
  TrieStorageImpl::getEphemeralBatchAt(const RootHash &root) const {
    SL_DEBUG(
        logger_, "Initialize ephemeral trie batch with root: {}", root.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}));
    return std::make_unique<EphemeralTrieBatchImpl>(codec_, std::move(trie));
  }
}  // namespace kagome::storage::trie

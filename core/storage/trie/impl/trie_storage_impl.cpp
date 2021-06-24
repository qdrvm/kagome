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
      boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes) {
    // will never be used, so content of the callback doesn't matter
    auto empty_trie = trie_factory->createEmpty(
        [](const auto &branch, auto idx) { return nullptr; });
    // ensure retrieval of empty trie succeeds
    OUTCOME_TRY(empty_root, serializer->storeTrie(*empty_trie));
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(empty_root),
                            std::move(codec),
                            std::move(serializer),
                            std::move(changes)));
  }

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createFromStorage(
      const RootHash &root_hash,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes) {
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(root_hash,
                            std::move(codec),
                            std::move(serializer),
                            std::move(changes)));
  }

  TrieStorageImpl::TrieStorageImpl(
      RootHash root_hash,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes)
      : root_hash_{std::move(root_hash)},
        codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        changes_{std::move(changes)},
        logger_{log::createLogger("TrieStorage", "storage")} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT((changes_.has_value() and changes_.value() != nullptr)
                 or not changes_.has_value());
    logger_->verbose("Initialize trie storage with root: {}", root_hash_.toHex());
  }

  outcome::result<std::unique_ptr<PersistentTrieBatch>>
  TrieStorageImpl::getPersistentBatch() {
    SL_DEBUG(logger_,
             "Initialize persistent trie batch with root: {}",
             root_hash_.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return PersistentTrieBatchImpl::create(
        codec_,
        serializer_,
        changes_,
        std::move(trie),
        [this](const auto &new_root) {
          root_hash_ = new_root;
          SL_DEBUG(logger_, "Update state root: {}", root_hash_);
        });
  }

  outcome::result<std::unique_ptr<EphemeralTrieBatch>>
  TrieStorageImpl::getEphemeralBatch() const {
    SL_DEBUG(logger_,
             "Initialize ephemeral trie batch with root: {}",
             root_hash_.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return std::make_unique<EphemeralTrieBatchImpl>(codec_, std::move(trie));
  }

  outcome::result<std::unique_ptr<PersistentTrieBatch>>
  TrieStorageImpl::getPersistentBatchAt(const RootHash &root) {
    SL_DEBUG(logger_,
             "Initialize persistent trie batch with root: {}",
             root.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}));
    return PersistentTrieBatchImpl::create(
        codec_,
        serializer_,
        changes_,
        std::move(trie),
        [this](const auto &new_root) {
          root_hash_ = new_root;
          SL_DEBUG(logger_, "Update state root: {}", root_hash_);
        });
  }

  outcome::result<std::unique_ptr<EphemeralTrieBatch>>
  TrieStorageImpl::getEphemeralBatchAt(const RootHash &root) const {
    SL_DEBUG(logger_,
             "Initialize ephemeral trie batch with root: {}",
             root_hash_.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}));
    return std::make_unique<EphemeralTrieBatchImpl>(codec_, std::move(trie));
  }

  RootHash TrieStorageImpl::getRootHash() const noexcept {
    return root_hash_;
  }
}  // namespace kagome::storage::trie

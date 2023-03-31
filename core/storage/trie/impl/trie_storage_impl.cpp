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
      std::shared_ptr<TrieSerializer> serializer) {
    // will never be used, so content of the callback doesn't matter
    auto empty_trie =
        trie_factory->createEmpty([](auto &) { return outcome::success(); });
    // ensure retrieval of empty trie succeeds
    OUTCOME_TRY(serializer->storeTrie(*empty_trie, StateVersion::V0));
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(codec), std::move(serializer)));
  }

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createFromStorage(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer) {
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(codec), std::move(serializer)));
  }

  TrieStorageImpl::TrieStorageImpl(std::shared_ptr<Codec> codec,
                                   std::shared_ptr<TrieSerializer> serializer)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        logger_{log::createLogger("TrieStorage", "storage")} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getPersistentBatchAt(const RootHash &root,
                                        TrieChangesTrackerOpt changes_tracker) {
    SL_DEBUG(logger_,
             "Initialize persistent trie batch with root: {}",
             root.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}, nullptr));
    return std::make_unique<PersistentTrieBatchImpl>(
        codec_, serializer_, std::move(changes_tracker), std::move(trie));
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getEphemeralBatchAt(const RootHash &root) const {
    SL_DEBUG(logger_, "Initialize ephemeral trie batch with root: {}", root);
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}, nullptr));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::move(trie), serializer_, nullptr);
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getProofReaderBatchAt(
      const RootHash &root, const OnNodeLoaded &on_node_loaded) const {
    SL_DEBUG(
        logger_, "Initialize proof reading trie batch with root: {}", root);
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root}, on_node_loaded));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::move(trie), serializer_, on_node_loaded);
  }

}  // namespace kagome::storage::trie

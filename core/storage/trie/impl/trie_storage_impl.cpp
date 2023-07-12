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
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner) {
    // will never be used, so content of the callback doesn't matter
    auto empty_trie =
        trie_factory->createEmpty();
    // ensure retrieval of empty trie succeeds
    OUTCOME_TRY(serializer->storeTrie(*empty_trie, StateVersion::V0));
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(codec),
                            std::move(serializer),
                            std::move(state_pruner)));
  }

  outcome::result<std::unique_ptr<TrieStorageImpl>>
  TrieStorageImpl::createFromStorage(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner) {
    return std::unique_ptr<TrieStorageImpl>(
        new TrieStorageImpl(std::move(codec),
                            std::move(serializer),
                            std::move(state_pruner)));
  }

  TrieStorageImpl::TrieStorageImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        state_pruner_{std::move(state_pruner)},
        logger_{log::createLogger("TrieStorage", "storage")} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(state_pruner_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getPersistentBatchAt(const RootHash &root,
                                        TrieChangesTrackerOpt changes_tracker) {
    SL_DEBUG(logger_,
             "Initialize persistent trie batch with root: {}",
             root.toHex());
    OUTCOME_TRY(trie, serializer_->retrieveTrie(root, nullptr));
    return std::make_unique<PersistentTrieBatchImpl>(
        codec_, serializer_, changes_tracker, std::move(trie), state_pruner_);
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getEphemeralBatchAt(const RootHash &root) const {
    SL_DEBUG(logger_, "Initialize ephemeral trie batch with root: {}", root);
    OUTCOME_TRY(trie, serializer_->retrieveTrie(root, nullptr));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::move(trie), serializer_, nullptr);
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  TrieStorageImpl::getProofReaderBatchAt(
      const RootHash &root, const OnNodeLoaded &on_node_loaded) const {
    SL_DEBUG(
        logger_, "Initialize proof reading trie batch with root: {}", root);
    OUTCOME_TRY(trie, serializer_->retrieveTrie(root, on_node_loaded));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::move(trie), serializer_, on_node_loaded);
  }

}  // namespace kagome::storage::trie

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

  TrieStorageImpl::TrieStorageImpl(
      common::Hash256 root_hash,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      std::shared_ptr<changes_trie::ChangesTracker> changes)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        changes_{std::move(changes)} {
    auto empty_root_hash = serializer_->getEmptyRootHash();
    if(std::equal(root_hash_ ) {

    }
  }

  outcome::result<std::unique_ptr<PersistentTrieBatch>>
  TrieStorageImpl::getPersistentBatch() {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return std::make_unique<PersistentTrieBatchImpl>(
        codec_,
        serializer_,
        changes_,
        std::shared_ptr(std::move(trie)),
        [this](auto const &new_root) {
          std::copy_n(new_root.begin(), root_hash_.size(), root_hash_.begin());
        });
  }

  outcome::result<std::unique_ptr<EphemeralTrieBatch>>
  TrieStorageImpl::getEphemeralBatch() const {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(Buffer{root_hash_}));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, std::shared_ptr(std::move(trie)));
  }

  common::Hash256 TrieStorageImpl::getRootHash() const {
    return root_hash_;
  }
}  // namespace kagome::storage::trie

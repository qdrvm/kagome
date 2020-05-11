/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/persistent_trie_batch_impl.hpp"

namespace kagome::storage::trie {

  PersistentTrieBatchImpl::PersistentTrieBatchImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
      std::shared_ptr<PolkadotTrie> trie,
      RootChangedEventHandler handler)
      : codec_{std::move(codec)},
        serializer_{std::move(serializer)},
        changes_{std::move(changes)},
        trie_{std::move(trie)},
        root_changed_handler_{std::move(handler)} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT((changes_.has_value() && changes_.value() != nullptr)
                 or not changes_.has_value());
    BOOST_ASSERT(trie_ != nullptr);
  }

  outcome::result<Buffer> PersistentTrieBatchImpl::commit() {
    OUTCOME_TRY(root, serializer_->storeTrie(*trie_));
    root_changed_handler_(root);
    return root;
  }

  outcome::result<Buffer> PersistentTrieBatchImpl::get(
      const Buffer &key) const {
    return trie_->get(key);
  }

  bool PersistentTrieBatchImpl::contains(const Buffer &key) const {
    return trie_->contains(key);
  }

  bool PersistentTrieBatchImpl::empty() const {
    return trie_->empty();
  }

  outcome::result<Buffer> PersistentTrieBatchImpl::calculateRoot() const {
    OUTCOME_TRY(enc, codec_->encodeNode(*trie_->getRoot()));
    return Buffer{codec_->hash256(enc)};
  }

  outcome::result<void> PersistentTrieBatchImpl::clearPrefix(
      const Buffer &prefix) {
    /// TODO(Harrm): notify changes tracker
    return trie_->clearPrefix(prefix);
  }

  outcome::result<void> PersistentTrieBatchImpl::put(const Buffer &key,
                                                     const Buffer &value) {
    auto res = trie_->put(key, value);
    if (res and changes_.has_value()) {
      changes_.value()->onChange(key);
    }
    return res;
  }

  outcome::result<void> PersistentTrieBatchImpl::put(const Buffer &key,
                                                     Buffer &&value) {
    auto res = trie_->put(key, std::move(value));
    if (res and changes_.has_value()) {
      changes_.value()->onChange(key);
    }
    return res;
  }

  outcome::result<void> PersistentTrieBatchImpl::remove(const Buffer &key) {
    auto res = trie_->remove(key);
    if (res and changes_.has_value()) {
      changes_.value()->onChange(key);
    }
    return res;
  }

}  // namespace kagome::storage::trie

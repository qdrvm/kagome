/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/persistent_trie_batch_impl.hpp"

#include "scale/scale.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

namespace kagome::storage::trie {

  const common::Buffer EXTRINSIC_INDEX_KEY =
      common::Buffer{}.put(":extrinsic_index");

  // sometimes there is no extrinsic index for a runtime call
  const common::Buffer NO_EXTRINSIC_INDEX_VALUE{
      scale::encode(0xffffffff).value()};

  PersistentTrieBatchImpl::PersistentTrieBatchImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieSerializer> serializer,
      boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
      std::unique_ptr<PolkadotTrie> trie,
      RootChangedEventHandler &&handler)
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
    if (changes_) {
      changes_.value()->setExtrinsicIdxGetter(
          [this]() -> outcome::result<Buffer> {
            auto res = trie_->get(EXTRINSIC_INDEX_KEY);
            if (res.has_error() and res.error() == TrieError::NO_VALUE) {
              return NO_EXTRINSIC_INDEX_VALUE;
            }
            return res;
          });
    }
  }

  outcome::result<Buffer> PersistentTrieBatchImpl::commit() {
    OUTCOME_TRY(root, serializer_->storeTrie(*trie_));
    root_changed_handler_(root);
    return std::move(root);
  }

  std::unique_ptr<TopperTrieBatch> PersistentTrieBatchImpl::batchOnTop() {
    return std::make_unique<TopperTrieBatchImpl>(shared_from_this());
  }

  outcome::result<Buffer> PersistentTrieBatchImpl::get(
      const Buffer &key) const {
    return trie_->get(key);
  }

  std::unique_ptr<BufferMapCursor> PersistentTrieBatchImpl::cursor() {
    return std::make_unique<PolkadotTrieCursor>(*trie_);
  }

  bool PersistentTrieBatchImpl::contains(const Buffer &key) const {
    return trie_->contains(key);
  }

  bool PersistentTrieBatchImpl::empty() const {
    return trie_->empty();
  }

  outcome::result<void> PersistentTrieBatchImpl::clearPrefix(
      const Buffer &prefix) {
    // TODO(Harrm): notify changes tracker
    return trie_->clearPrefix(prefix);
  }

  outcome::result<void> PersistentTrieBatchImpl::put(const Buffer &key,
                                                     const Buffer &value) {
    return put(key, Buffer{value});  // would have to copy anyway
  }

  outcome::result<void> PersistentTrieBatchImpl::put(const Buffer &key,
                                                     Buffer &&value) {
    bool is_new_entry = not trie_->contains(key);
    auto res = trie_->put(key, value);
    if (res and changes_.has_value()) {
      OUTCOME_TRY(changes_.value()->onPut(key, value, is_new_entry));
    }
    return res;
  }

  outcome::result<void> PersistentTrieBatchImpl::remove(const Buffer &key) {
    auto res = trie_->remove(key);
    if (res and changes_.has_value()) {
      OUTCOME_TRY(changes_.value()->onRemove(key));
    }
    return res;
  }

}  // namespace kagome::storage::trie

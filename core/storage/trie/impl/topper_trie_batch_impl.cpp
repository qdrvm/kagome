/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/topper_trie_batch_impl.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include "common/buffer.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            TopperTrieBatchImpl::Error,
                            e) {
  using E = kagome::storage::trie::TopperTrieBatchImpl::Error;
  switch (e) {
    case E::PARENT_EXPIRED:
      return "Pointer to the parent batch expired";
    case E::CHILD_BATCH_NOT_SUPPORTED:
      return "Topper trie batches do not support child trie batch creation";
    case E::COMMIT_NOT_SUPPORTED:
      return "Topper trie batches do not support committing changes, use "
             "writeBack instead";
    case E::CURSOR_NEXT_INVALID:
      return "TopperTrieCursor::next() called on invalid cursor";
    case E::CURSOR_SEEK_LAST_NOT_IMPLEMENTED:
      return "TopperTrieCursor::seekLast() not implemented";
    case E::CURSOR_PREV_NOT_IMPLEMENTED:
      return "TopperTrieCursor::prev() not implemented";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  TopperTrieBatchImpl::TopperTrieBatchImpl(
      const std::shared_ptr<TrieBatch> &parent)
      : parent_(parent) {}

  outcome::result<BufferOrView> TopperTrieBatchImpl::get(
      const BufferView &key) const {
    OUTCOME_TRY(opt_value, tryGet(key));
    if (opt_value) {
      return std::move(*opt_value);
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<std::optional<BufferOrView>> TopperTrieBatchImpl::tryGet(
      const BufferView &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      if (it->second.has_value()) {
        return BufferView{it->second.value()};
      }
      return std::nullopt;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->tryGet(key);
    }
    return Error::PARENT_EXPIRED;
  }

  std::unique_ptr<PolkadotTrieCursor> TopperTrieBatchImpl::trieCursor() {
    if (auto p = parent_.lock(); p != nullptr) {
      return std::make_unique<TopperTrieCursor>(shared_from_this(),
                                                p->trieCursor());
    }
    return nullptr;
  }

  outcome::result<bool> TopperTrieBatchImpl::contains(
      const BufferView &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      return it->second.has_value();
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->contains(key);
    }
    return false;
  }

  outcome::result<void> TopperTrieBatchImpl::put(const BufferView &key,
                                                 BufferOrView &&value) {
    cache_.insert_or_assign(Buffer{key}, value.intoBuffer());
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::remove(const BufferView &key) {
    cache_.insert_or_assign(Buffer{key}, std::nullopt);

    return outcome::success();
  }

  outcome::result<std::tuple<bool, uint32_t>> TopperTrieBatchImpl::clearPrefix(
      const BufferView &prefix, std::optional<uint64_t>) {
    for (auto it = cache_.lower_bound(prefix);
         it != cache_.end() && startsWith(it->first, prefix);
         ++it) {
      it->second = std::nullopt;
    }

    if (parent_.lock() != nullptr) {
      return outcome::success(std::make_tuple(false, 0ULL));
    }
    return Error::PARENT_EXPIRED;
  }

  outcome::result<void> TopperTrieBatchImpl::writeBack() {
    if (auto p = parent_.lock()) {
      return apply(*p);
    }
    return Error::PARENT_EXPIRED;
  }

  outcome::result<void> TopperTrieBatchImpl::apply(
      storage::BufferStorage &map) {
    for (auto &[k, v] : cache_) {
      if (v) {
        OUTCOME_TRY(map.put(k, BufferView{*v}));
      } else {
        OUTCOME_TRY(map.remove(k));
      }
    }
    return outcome::success();
  }

  outcome::result<RootHash> TopperTrieBatchImpl::commit(StateVersion version) {
    return Error::COMMIT_NOT_SUPPORTED;
  }

  outcome::result<std::optional<std::shared_ptr<TrieBatch>>>
  TopperTrieBatchImpl::createChildBatch(common::BufferView path) {
    return Error::CHILD_BATCH_NOT_SUPPORTED;
  }

  TopperTrieCursor::TopperTrieCursor(std::shared_ptr<TopperTrieBatchImpl> batch,
                                     std::unique_ptr<PolkadotTrieCursor> cursor)
      : parent_batch_{std::move(batch)},
        parent_cursor_{std::move(cursor)},
        overlay_it_{parent_batch_->cache_.end()} {}

  outcome::result<bool> TopperTrieCursor::seekFirst() {
    OUTCOME_TRY(parent_cursor_->seekFirst());
    cached_parent_key_ = parent_cursor_->key();
    overlay_it_ = parent_batch_->cache_.begin();
    choose();
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  outcome::result<bool> TopperTrieCursor::seek(const BufferView &key) {
    OUTCOME_TRY(seekLowerBound(key));
    return isValid();
  }

  outcome::result<bool> TopperTrieCursor::seekLast() {
    return TopperTrieBatchImpl::Error::CURSOR_SEEK_LAST_NOT_IMPLEMENTED;
  }

  bool TopperTrieCursor::isValid() const {
    return choice_;
  }

  outcome::result<void> TopperTrieCursor::next() {
    OUTCOME_TRY(step());
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  outcome::result<void> TopperTrieCursor::prev() {
    return TopperTrieBatchImpl::Error::CURSOR_PREV_NOT_IMPLEMENTED;
  }

  std::optional<Buffer> TopperTrieCursor::key() const {
    return choice_.overlay ? overlay_it_->first : cached_parent_key_;
  }

  std::optional<BufferOrView> TopperTrieCursor::value() const {
    return choice_.overlay ? Buffer{*overlay_it_->second}
                           : parent_cursor_->value();
  }

  outcome::result<void> TopperTrieCursor::seekLowerBound(
      const BufferView &key) {
    OUTCOME_TRY(parent_cursor_->seekLowerBound(key));
    cached_parent_key_ = parent_cursor_->key();
    overlay_it_ = parent_batch_->cache_.lower_bound(key);
    choose();
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  outcome::result<void> TopperTrieCursor::seekUpperBound(
      const BufferView &key) {
    OUTCOME_TRY(parent_cursor_->seekUpperBound(key));
    cached_parent_key_ = parent_cursor_->key();
    overlay_it_ = parent_batch_->cache_.upper_bound(key);
    choose();
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  void TopperTrieCursor::choose() {
    if (overlay_it_ != parent_batch_->cache_.end()
        and (not cached_parent_key_
             or *cached_parent_key_ >= overlay_it_->first)) {
      choice_ = Choice{cached_parent_key_ == overlay_it_->first, true};
      return;
    }
    if (cached_parent_key_) {
      choice_ = Choice{true, false};
    } else {
      choice_ = Choice{false, false};
    }
  }

  bool TopperTrieCursor::isRemoved() const {
    if (not choice_) {
      return false;
    }
    if (choice_.overlay) {
      return not overlay_it_->second;
    }
    return false;
  }

  outcome::result<void> TopperTrieCursor::skipRemoved() {
    while (isRemoved()) {
      OUTCOME_TRY(step());
    }
    return outcome::success();
  }

  outcome::result<void> TopperTrieCursor::step() {
    if (not choice_) {
      return TopperTrieBatchImpl::Error::CURSOR_NEXT_INVALID;
    }
    if (choice_.parent) {
      OUTCOME_TRY(parent_cursor_->next());
      cached_parent_key_ = parent_cursor_->key();
    }
    if (choice_.overlay) {
      ++overlay_it_;
    }
    choose();
    return outcome::success();
  }
}  // namespace kagome::storage::trie

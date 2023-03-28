/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    if (wasClearedByPrefix(key)) {
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
    if (wasClearedByPrefix(key)) {
      return false;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->contains(key);
    }
    return false;
  }

  bool TopperTrieBatchImpl::empty() const {
    if (not cache_.empty()
        and std::any_of(cache_.begin(), cache_.end(), [](auto &p) {
              return p.second.has_value();
            })) {
      return false;
    }
    // TODO(Harrm) PRE-462 consider clearPrefix here. Not an easy thing and is
    // barely possible to happen, so leave it for the future
    if (auto p = parent_.lock(); p != nullptr) {
      return p->empty();
    }
    return true;
  }

  outcome::result<void> TopperTrieBatchImpl::put(const BufferView &key,
                                                 BufferOrView &&value) {
    cache_.insert_or_assign(Buffer{key}, value.into());
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::remove(const BufferView &key) {
    cache_.insert_or_assign(Buffer{key}, std::nullopt);

    return outcome::success();
  }

  outcome::result<std::tuple<bool, uint32_t>> TopperTrieBatchImpl::clearPrefix(
      const BufferView &prefix, std::optional<uint64_t>) {
    for (auto it = cache_.lower_bound(prefix);
         it != cache_.end() && boost::starts_with(it->first, prefix);
         ++it) {
      it->second = std::nullopt;
    }

    cleared_prefixes_.emplace_back(prefix);
    if (parent_.lock() != nullptr) {
      return outcome::success(std::make_tuple(true, 0ULL));
    }
    return Error::PARENT_EXPIRED;
  }

  outcome::result<void> TopperTrieBatchImpl::writeBack() {
    if (auto p = parent_.lock()) {
      for (const auto &prefix : cleared_prefixes_) {
        OUTCOME_TRY(p->clearPrefix(prefix));
      }
      for (auto it = cache_.begin(); it != cache_.end(); it++) {
        if (it->second.has_value()) {
          OUTCOME_TRY(p->put(it->first, BufferView{it->second.value()}));
        } else {
          OUTCOME_TRY(p->remove(it->first));
        }
      }
      return outcome::success();
    }
    return Error::PARENT_EXPIRED;
  }

  outcome::result<RootHash> TopperTrieBatchImpl::commit(StateVersion version) {
    return Error::COMMIT_NOT_SUPPORTED;
  }

  outcome::result<std::optional<std::shared_ptr<TrieBatch>>>
  TopperTrieBatchImpl::createChildBatch(common::BufferView path) {
    return Error::CHILD_BATCH_NOT_SUPPORTED;
  }

  bool TopperTrieBatchImpl::wasClearedByPrefix(const BufferView &key) const {
    for (const auto &prefix : cleared_prefixes_) {
      if (boost::starts_with(key, prefix)) {
        return true;
      }
    }
    return false;
  }

  TopperTrieCursor::TopperTrieCursor(std::shared_ptr<TopperTrieBatchImpl> batch,
                                     std::unique_ptr<PolkadotTrieCursor> cursor)
      : batch_{std::move(batch)},
        cursor_{std::move(cursor)},
        it_{batch_->cache_.end()} {}

  outcome::result<bool> TopperTrieCursor::seekFirst() {
    OUTCOME_TRY(cursor_->seekFirst());
    cursor_key_ = cursor_->key();
    it_ = batch_->cache_.begin();
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
    return is_it_eq_.has_value();
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
    return is_it_eq_ and *is_it_eq_ ? it_->first : cursor_key_;
  }

  std::optional<BufferOrView> TopperTrieCursor::value() const {
    return is_it_eq_ and *is_it_eq_ ? Buffer{*it_->second} : cursor_->value();
  }

  outcome::result<void> TopperTrieCursor::seekLowerBound(
      const BufferView &key) {
    OUTCOME_TRY(cursor_->seekLowerBound(key));
    cursor_key_ = cursor_->key();
    it_ = batch_->cache_.lower_bound(key);
    choose();
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  outcome::result<void> TopperTrieCursor::seekUpperBound(
      const BufferView &key) {
    OUTCOME_TRY(cursor_->seekUpperBound(key));
    cursor_key_ = cursor_->key();
    it_ = batch_->cache_.upper_bound(key);
    choose();
    OUTCOME_TRY(skipRemoved());
    return outcome::success();
  }

  void TopperTrieCursor::choose() {
    if (it_ != batch_->cache_.end()
        and (not cursor_key_ or *cursor_key_ >= it_->first)) {
      is_it_eq_ = cursor_key_ == it_->first;
      return;
    }
    if (cursor_key_) {
      is_it_eq_ = std::optional<bool>{};
    } else {
      is_it_eq_.reset();
    }
  }

  bool TopperTrieCursor::isRemoved() const {
    if (not is_it_eq_) {
      return false;
    }
    if (*is_it_eq_) {
      return not it_->second;
    }
    return batch_->wasClearedByPrefix(*cursor_key_);
  }

  outcome::result<void> TopperTrieCursor::skipRemoved() {
    while (isRemoved()) {
      OUTCOME_TRY(step());
    }
    return outcome::success();
  }

  outcome::result<void> TopperTrieCursor::step() {
    if (not is_it_eq_) {
      return TopperTrieBatchImpl::Error::CURSOR_NEXT_INVALID;
    }
    if (not *is_it_eq_ or **is_it_eq_) {
      OUTCOME_TRY(cursor_->next());
      cursor_key_ = cursor_->key();
    }
    if (*is_it_eq_) {
      ++it_;
    }
    choose();
    return outcome::success();
  }
}  // namespace kagome::storage::trie

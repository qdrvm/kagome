/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/topper_trie_batch_impl.hpp"

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
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  TopperTrieBatchImpl::TopperTrieBatchImpl(
      const std::shared_ptr<TrieBatch> &parent)
      : parent_(parent) {}

  outcome::result<Buffer> TopperTrieBatchImpl::get(const Buffer &key) const {
    OUTCOME_TRY(opt_value, tryGet(key));
    if (opt_value) {
      return opt_value.value();
    }
    return TrieError::NO_VALUE;
  }

  outcome::result<boost::optional<Buffer>> TopperTrieBatchImpl::tryGet(
      const Buffer &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      if (it->second.has_value()) {
        return it->second.value();
      }
      return boost::none;
    }
    if (wasClearedByPrefix(key)) {
      return boost::none;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->tryGet(key);
    }
    return Error::PARENT_EXPIRED;
  }

  std::unique_ptr<PolkadotTrieCursor> TopperTrieBatchImpl::trieCursor() {
    if (auto p = parent_.lock(); p != nullptr) {
      return p->trieCursor();
    }
    return nullptr;
  }

  bool TopperTrieBatchImpl::contains(const Buffer &key) const {
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

  outcome::result<void> TopperTrieBatchImpl::put(const Buffer &key,
                                                 const Buffer &value) {
    return put(key, Buffer(value));
  }

  outcome::result<void> TopperTrieBatchImpl::put(const Buffer &key,
                                                 Buffer &&value) {
    cache_[key] = std::move(value);
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::remove(const Buffer &key) {
    cache_[key] = boost::none;
    return outcome::success();
  }

  outcome::result<std::tuple<bool, uint32_t>> TopperTrieBatchImpl::clearPrefix(
      const Buffer &prefix, boost::optional<uint64_t>) {
    for (auto it = cache_.lower_bound(prefix);
         it != cache_.end() && it->first.subbuffer(0, prefix.size()) == prefix;
         ++it)
      it->second = boost::none;

    cleared_prefixes_.push_back(prefix);
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
          OUTCOME_TRY(p->put(it->first, it->second.value()));
        } else {
          OUTCOME_TRY(p->remove(it->first));
        }
      }
      return outcome::success();
    }
    return Error::PARENT_EXPIRED;
  }

  bool TopperTrieBatchImpl::wasClearedByPrefix(const Buffer &key) const {
    for (const auto &prefix : cleared_prefixes_) {
      auto key_end = key.begin();
      std::advance(key_end, std::min(key.size(), prefix.size()) - 1);
      auto is_cleared = std::equal(key.begin(), key_end, prefix.begin());
      if (is_cleared) return true;
    }
    return false;
  }

}  // namespace kagome::storage::trie

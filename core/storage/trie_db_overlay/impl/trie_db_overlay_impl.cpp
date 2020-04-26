/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_db_overlay/impl/trie_db_overlay_impl.hpp"

#include "scale/scale.hpp"
#include "storage/trie/impl/trie_error.hpp"

namespace kagome::storage::trie_db_overlay {

  const common::Buffer TrieDbOverlayImpl::EXTRINSIC_INDEX_KEY{[]() {
    constexpr auto s = ":extrinsic_index";
    std::vector<uint8_t> v(strlen(s));
    std::copy(s, s + strlen(s), v.begin());
    return v;
  }()};

  TrieDbOverlayImpl::TrieDbOverlayImpl(
      std::shared_ptr<trie::TrieDb> main_db,
      std::shared_ptr<trie::TrieDbFactory> cache_storage_factory)
      : storage_{std::move(main_db)},
        cache_factory_ {std::move(cache_storage_factory)},
        logger_{common::createLogger("TrieDb Overlay")} {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(cache_factory_ != nullptr);
    cache_ = cache_factory_->makeTrieDb();
  }

  outcome::result<void> TrieDbOverlayImpl::commit() {
    for (auto &change : extrinsics_changes_) {
      if (not change.second.dirty) {
        continue;
      }
      change.second.dirty = false;
    }
    OUTCOME_TRY(storeCache());
    cache_ = cache_factory_->makeTrieDb();
    logger_->debug("Commit changes from overlay to the main storage");
    return outcome::success();
  }

  outcome::result<void> TrieDbOverlayImpl::sinkChangesTo(
      blockchain::ChangesTrieBuilder &changes_trie) {
    OUTCOME_TRY(commit());
    for (auto &change : extrinsics_changes_) {
      auto &key = change.first;
      OUTCOME_TRY(
          changes_trie.insertExtrinsicsChange(key, change.second.changers));
    }
    extrinsics_changes_.clear();
    return outcome::success();
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>> TrieDbOverlayImpl::batch() {
    BOOST_ASSERT_MSG(false, "Not implemented");
    return nullptr;
  }

  std::unique_ptr<face::MapCursor<Buffer, Buffer>> TrieDbOverlayImpl::cursor() {
    BOOST_ASSERT_MSG(false, "Not implemented");
    return nullptr;
  }

  outcome::result<Buffer> TrieDbOverlayImpl::get(const Buffer &key) const {
    if (auto it = extrinsics_changes_.find(key);
        it != extrinsics_changes_.end()) {
      auto v_opt = it->second.value;
      if (v_opt.has_value()) {
        return v_opt.value();
      } else {
        return trie::TrieError::NO_VALUE;
      }
    }
    return storage_->get(key);
  }

  bool TrieDbOverlayImpl::contains(const Buffer &key) const {
    auto it = extrinsics_changes_.find(key);
    if (it != extrinsics_changes_.end()) {
      return it->second.value.has_value();
    }
    return storage_->contains(key);
  }

  outcome::result<void> TrieDbOverlayImpl::put(const Buffer &key,
                                               const Buffer &value) {
    return put(key, Buffer{value});  // would have to copy anyway
  }

  outcome::result<void> TrieDbOverlayImpl::put(const Buffer &key,
                                               Buffer &&value) {
    auto extrinsic_id = getExtrinsicIndex();
    auto it = extrinsics_changes_.find(key);
    if (it != extrinsics_changes_.end()) {
      it->second.value = std::move(value);
      it->second.changers.emplace_back(extrinsic_id);
      it->second.dirty = true;
    } else {
      extrinsics_changes_[key] = ChangedValue{
          .value = std::move(value), .changers = {extrinsic_id}, .dirty = true};
    }

    return outcome::success();
  }

  outcome::result<void> TrieDbOverlayImpl::remove(const Buffer &key) {
    auto extrinsic_id = getExtrinsicIndex();
    auto it = extrinsics_changes_.find(key);
    if (it != extrinsics_changes_.end()) {
      it->second.value = boost::none;
      it->second.dirty = true;
      it->second.changers.emplace_back(extrinsic_id);
    } else {
      extrinsics_changes_[key] = ChangedValue{
          .value = boost::none, .changers = {extrinsic_id}, .dirty = true};
    }

    return outcome::success();
  }

  outcome::result<void> TrieDbOverlayImpl::clearPrefix(
      const common::Buffer &prefix) {
    auto begin = extrinsics_changes_.lower_bound(buf);
    auto is_prefix = [](auto &prefix, auto &buf) {
      if (buf.size() < prefix.size()) return false;
      return std::equal(prefix.begin(), prefix.end(), buf.begin());
    };
    auto it = begin;
    for (; it != extrinsics_changes_.end(); it++) {
      if (not is_prefix(prefix, *it)) {
        break;
      }
      // remove element
      it->second.value = boost::none;
      it->second.dirty = true;
      it->second.changers.emplace_back(extrinsic_id);
    }
    return storage_->clearPrefix(prefix);
  }

  Buffer TrieDbOverlayImpl::getRootHash() {
    commit();
    return storage_->getRootHash();
  }

  bool TrieDbOverlayImpl::empty() const {
    // gets a bit complex because some of the entries in changes are actually
    // deleted entries
    bool any_nondeleted_change = std::any_of(
        extrinsics_changes_.begin(),
        extrinsics_changes_.end(),
        [](auto &change) { return change.second.value.has_value(); });
    /// TODO(harrm): check storage not cleared completely in cached chages
    /// considering temporary values
    return any_nondeleted_change or not storage_->empty();
  }

  primitives::ExtrinsicIndex TrieDbOverlayImpl::getExtrinsicIndex() const {
    auto ext_idx_bytes_res = get(EXTRINSIC_INDEX_KEY);
    if (ext_idx_bytes_res.has_value()) {
      auto ext_idx_res =
          scale::decode<primitives::ExtrinsicIndex>(ext_idx_bytes_res.value());
      BOOST_ASSERT_MSG(ext_idx_res.has_value(),
                       "Extrinsic index decoding must not fail");
      logger_->error("Extrinsic index decoding failed, which must not happen");
      return ext_idx_res.value();
    }
    return NO_EXTRINSIC_INDEX;
  }

}  // namespace kagome::storage::trie_db_overlay

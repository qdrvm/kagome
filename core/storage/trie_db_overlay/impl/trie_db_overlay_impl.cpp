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

  TrieDbOverlayImpl::TrieDbOverlayImpl(std::shared_ptr<trie::TrieDb> main_db)
      : storage_{std::move(main_db)} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<void> TrieDbOverlayImpl::commitAndInsertChanges(
      blockchain::ChangesTrie &changes_trie) {
    for (auto &change : changes_) {
      if (change.second.value.has_value()) {
        OUTCOME_TRY(storage_->put(change.first, change.second.value.value()));
      } else {
        OUTCOME_TRY(storage_->remove(change.first));
      }

      /// TODO(Harrm): insert every change that is not temporary to changes trie
    }
    changes_.clear();
    return outcome::success();
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>> TrieDbOverlayImpl::batch() {
    return nullptr;
  }

  std::unique_ptr<face::MapCursor<Buffer, Buffer>> TrieDbOverlayImpl::cursor() {
    return nullptr;
  }

  outcome::result<Buffer> TrieDbOverlayImpl::get(const Buffer &key) const {
    if (auto it = changes_.find(key); it != changes_.end()) {
      return it->second.value.value_or(trie::TrieError::NO_VALUE);
    }
    return storage_->get(key);
  }

  bool TrieDbOverlayImpl::contains(const Buffer &key) const {
    auto it = changes_.find(key);
    if (it != changes_.end()) {
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
    auto it = changes_.find(key);
    it->second.value = std::move(value);
    it->second.changers.emplace_back(extrinsic_id);
    return outcome::success();
  }

  outcome::result<void> TrieDbOverlayImpl::remove(const Buffer &key) {
    auto extrinsic_id = getExtrinsicIndex();
    auto it = changes_.find(key);
    it->second.value = boost::none;
    it->second.changers.emplace_back(extrinsic_id);
    return outcome::success();
  }

  outcome::result<void> TrieDbOverlayImpl::clearPrefix(
      const common::Buffer &buf) {
    return outcome::result<void>();
  }

  Buffer TrieDbOverlayImpl::getRootHash() {
    /// TODO(Harrm): process commit resuly; maybe not inherit overlay from trie
    /// db after all?
    commit();
    return storage_->getRootHash();
  }

  bool TrieDbOverlayImpl::empty() const {
    // gets a bit complex because some of the entries in changes are actually
    // deleted entries
    bool any_nondeleted_change =
        std::any_of(changes_.begin(), changes_.end(), [](auto &change) {
          return change.second.value.has_value();
        });
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
      return ext_idx_res.value();
    }
    return NO_EXTRINSIC_INDEX;
  }

}  // namespace kagome::storage::trie_db_overlay

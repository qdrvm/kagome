/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/readonly_trie_db_backend.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, ReadonlyTrieDbBackend::Error, e) {
  using E = kagome::storage::trie::ReadonlyTrieDbBackend::Error;
  switch (e) {
    case E::CHANGE_ROOT_OF_READONLY_TRIE:
      return "Attempting to change root hash record of a read-only trie";
    case E::WRITE_TO_READONLY_TRIE:
      return "Attempting to modify a read-only trie";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  ReadonlyTrieDbBackend::ReadonlyTrieDbBackend(
      std::shared_ptr<PersistentBufferMap> storage,
      common::Hash256 root_hash,
      common::Buffer node_prefix)
      : TrieDbBackend{std::move(node_prefix)},
        storage_{std::move(storage)},
        root_hash_{root_hash} {
    BOOST_ASSERT(storage_ != nullptr);
  }

  std::unique_ptr<face::MapCursor<Buffer, Buffer>>
  ReadonlyTrieDbBackend::cursor() {
    return storage_
        ->cursor();  // TODO(Harrm): perhaps should iterate over trie nodes only
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>>
  ReadonlyTrieDbBackend::batch() {
    return nullptr;
  }

  outcome::result<Buffer> ReadonlyTrieDbBackend::get(const Buffer &key) const {
    return storage_->get(prefixKey(key));
  }

  bool ReadonlyTrieDbBackend::contains(const Buffer &key) const {
    return storage_->contains(prefixKey(key));
  }

  outcome::result<void> ReadonlyTrieDbBackend::put(const Buffer &key,
                                                   const Buffer &value) {
    return Error::WRITE_TO_READONLY_TRIE;
  }

  outcome::result<void> ReadonlyTrieDbBackend::put(const Buffer &key,
                                                   Buffer &&value) {
    return Error::WRITE_TO_READONLY_TRIE;
  }

  outcome::result<void> ReadonlyTrieDbBackend::remove(const Buffer &key) {
    return Error::WRITE_TO_READONLY_TRIE;
  }

  outcome::result<void> ReadonlyTrieDbBackend::saveRootHash(
      const common::Buffer &h) {
    return Error::CHANGE_ROOT_OF_READONLY_TRIE;
  }

  outcome::result<common::Buffer> ReadonlyTrieDbBackend::getRootHash() const {
    return root_hash_;
  }

}  // namespace kagome::storage::trie

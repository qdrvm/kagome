/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/readonly_trie_db_backend.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, ReadonlyTrieDbBackend, e) {
  using E = kagome::storage::trie::ReadonlyTrieDbBackend::Error;
  switch(e) {
    case E::CHANGE_ROOT_OF_READONLY_TRIE:
      return "Attempting to change root hash record of a read-only trie";
    case E::WRITE_BATCH_OF_READONLY_TRIE:
      return "Attempting to obtain a write batch to a read-only trie";
    case E::WRITE_TO_READONLY_TRIE:
      return "Attempting to modify a read-only trie";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  std::unique_ptr<face::MapCursor<Buffer, Buffer>>
  ReadonlyTrieDbBackend::cursor() {
    return std::unique_ptr<face::MapCursor<Buffer, Buffer>>();
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>>
  ReadonlyTrieDbBackend::batch() {
    return std::unique_ptr<face::WriteBatch<Buffer, Buffer>>();
  }

  outcome::result<Buffer> ReadonlyTrieDbBackend::get(const Buffer &key) const {
    return outcome::result<Buffer>();
  }

  bool ReadonlyTrieDbBackend::contains(const Buffer &key) const {
    return false;
  }

  outcome::result<void> ReadonlyTrieDbBackend::put(const Buffer &key,
                                                   const Buffer &value) {
    return outcome::result<void>();
  }

  outcome::result<void> ReadonlyTrieDbBackend::put(const Buffer &key,
                                                   Buffer &&value) {
    return outcome::result<void>();
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

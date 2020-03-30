/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_db_overlay/impl/polkadot_trie_db_overlay_impl.hpp"

namespace kagome::storage::trie_db_overlay {

  std::unique_ptr<PolkadotTrieDbOverlayImpl> PolkadotTrieDbOverlayImpl::createFromStorage(common::Buffer root,
                                                                    std::shared_ptr<trie::TrieDbBackend> backend) {
    BOOST_ASSERT(backend != nullptr);
    PolkadotTrieDbOverlayImpl trie_db{std::move(backend), std::move(root)};
    return std::make_unique<PolkadotTrieDbOverlayImpl>(std::move(trie_db));
  }

  std::unique_ptr<PolkadotTrieDbOverlayImpl> PolkadotTrieDbOverlayImpl::createEmpty(
      std::shared_ptr<trie::TrieDbBackend> backend) {
    BOOST_ASSERT(backend != nullptr);
    PolkadotTrieDbOverlayImpl trie_db{std::move(backend), boost::none};
    return std::make_unique<PolkadotTrieDbOverlayImpl>(std::move(trie_db));
  }

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>>
  PolkadotTrieDbOverlayImpl::batch() {
    return std::unique_ptr<face::WriteBatch<Buffer, Buffer>>();
  }

  std::unique_ptr<face::MapCursor<Buffer, Buffer>>
  PolkadotTrieDbOverlayImpl::cursor() {
    return std::unique_ptr<face::MapCursor<Buffer, Buffer>>();
  }

  outcome::result<Buffer> PolkadotTrieDbOverlayImpl::get(const Buffer &key) const {
    return outcome::result<Buffer>();
  }

  bool PolkadotTrieDbOverlayImpl::contains(const Buffer &key) const {
    return false;
  }

  outcome::result<void> PolkadotTrieDbOverlayImpl::put(const Buffer &key,
                                               const Buffer &value) {
    return outcome::result<void>();
  }

  outcome::result<void> PolkadotTrieDbOverlayImpl::put(const Buffer &key,
                                               Buffer &&value) {
    return outcome::result<void>();
  }

  outcome::result<void> PolkadotTrieDbOverlayImpl::remove(const Buffer &key) {
    return outcome::result<void>();
  }

  outcome::result<void> PolkadotTrieDbOverlayImpl::clearPrefix(
      const common::Buffer &buf) {
    return outcome::result<void>();
  }

  Buffer PolkadotTrieDbOverlayImpl::getRootHash() const {
    return Buffer();
  }

  bool PolkadotTrieDbOverlayImpl::empty() const {
    return false;
  }

}  // namespace kagome::storage::trie_db_overlay

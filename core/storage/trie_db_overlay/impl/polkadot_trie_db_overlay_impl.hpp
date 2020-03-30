/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_DB_OVERLAY_IMPL_HPP
#define KAGOME_STORAGE_TRIE_DB_OVERLAY_IMPL_HPP

#include "storage/trie/impl/polkadot_trie_db.hpp"
#include "storage/trie_db_overlay/trie_db_overlay.hpp"

namespace kagome::storage::trie_db_overlay {

  class PolkadotTrieDbOverlayImpl : public TrieDbOverlay,
                                    public trie::PolkadotTrieDb {
   public:
    PolkadotTrieDbOverlayImpl(ExtrinsicIdAccessor ext_id_accessor,
                              std::shared_ptr<trie::TrieDbBackend> db,
                              boost::optional<common::Buffer> root_hash);
    ~PolkadotTrieDbOverlayImpl() override;

    virtual void commitAndSinkTo(ChangesTrie &changes_trie) = 0;

    std::unique_ptr<face::WriteBatch<Buffer, Buffer>> batch() override;
    std::unique_ptr<face::MapCursor<Buffer, Buffer>> cursor() override;
    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;
    outcome::result<void> clearPrefix(const common::Buffer &buf) override;
    Buffer getRootHash() const override;
    bool empty() const override;

   private:
    using ExtrinsicIndex = uint32_t;

    std::vector<std::tuple<ExtrinsicIndex, common::Buffer, common::Buffer>>
        changes_;
    trie::PolkadotTrie cache_;
  };

}  // namespace kagome::storage::trie_db_overlay

#endif  // KAGOME_STORAGE_TRIE_DB_OVERLAY_IMPL_HPP

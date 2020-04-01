/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

#include <common/blob.hpp>

#include "blockchain/changes_trie.hpp"
#include "blockchain/changes_trie_config.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::blockchain {

  class ChangesTrieImpl : public ChangesTrie {
   public:
    ChangesTrieImpl(common::Hash256 parent,
                    ChangesTrieConfig config,
                    std::unique_ptr<storage::trie::TrieDb> changes_storage);

    outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) override;

    // outcome::result<void> insertBlocksChange() override;

    common::Buffer getRootHash() const override;

    void clean();

   private:
    common::Hash256 parent_;
    ChangesTrieConfig config_;
    std::unique_ptr<storage::trie::TrieDb> changes_storage_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

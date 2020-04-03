/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

#include "blockchain/changes_trie_builder.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/trie/trie_db_factory.hpp"

namespace kagome::blockchain {

  class ChangesTrieBuilderImpl : public ChangesTrieBuilder {
   public:
    explicit ChangesTrieBuilderImpl(
        common::Hash256 parent,
        ChangesTrieConfig config,
        std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory);

    ChangesTrieBuilder &startNewTrie(
        primitives::BlockHash parent,
        boost::optional<ChangesTrieConfig> config) override;

    outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) override;

    // outcome::result<void> insertBlocksChange() override;

    common::Hash256 finishAndGetHash() override;

   private:
    primitives::BlockHash parent_;
    ChangesTrieConfig config_;
    std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory_;
    std::unique_ptr<storage::trie::TrieDb> changes_storage_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

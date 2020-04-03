/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

#include <boost/variant.hpp>

#include "blockchain/block_header_repository.hpp"
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
        std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo);

    ChangesTrieBuilder &startNewTrie(
        primitives::BlockHash parent,
        boost::optional<ChangesTrieConfig> config) override;

    outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) override;

    // outcome::result<void> insertBlocksChange() override;

    common::Hash256 finishAndGetHash() override;

   private:
    struct KeyIndex {
      primitives::BlockNumber block;
      common::Buffer key;
    };
    // Mapping between storage key and extrinsics
    struct ExtrinsicsChangesKey : public KeyIndex {};
    // Mapping between storage key and blocks
    struct BlocksChangesKey : public KeyIndex {};
    //  Mapping between storage key and child changes Trie
    struct ChildChangesKey : public KeyIndex {};

    // the key used for the Changes Trie must be the varying datatype, not the
    // individual, appended KeyIndex.
    // Unlike the default encoding for varying data types, this
    // structure starts its indexing at 1
    using KeyIndexVariant = boost::variant<uint32_t,
                                           ExtrinsicsChangesKey,
                                           BlocksChangesKey,
                                           ChildChangesKey>;

    template <class Stream,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    friend Stream &operator<<(Stream &s, const KeyIndex &b) {
      return s << b.block << b.key;
    }
    template <class Stream,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    friend Stream &operator>>(Stream &s, KeyIndex &b) {
      return s >> b.block >> b.key;
    }

    primitives::BlockHash parent_;
    ChangesTrieConfig config_;
    std::shared_ptr<storage::trie::TrieDbFactory> changes_storage_factory_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::unique_ptr<storage::trie::TrieDb> changes_storage_;
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

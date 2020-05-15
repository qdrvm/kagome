/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

#include <boost/variant.hpp>

#include "blockchain/block_header_repository.hpp"
#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/changes_trie/changes_trie_builder.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie/impl/polkadot_trie_factory.hpp"
#include "storage/trie/codec.hpp"
#include "common/logger.hpp"

namespace kagome::storage::changes_trie {

  class ChangesTrieBuilderImpl : public ChangesTrieBuilder {
   public:
    enum class Error { TRIE_NOT_INITIALIZED = 1 };

    /**
     * @param storage - the node main storage
     * @param changes_storage_factory - trie db factory to make storage for the
     * generated changes trie
     * @param block_header_repo - the node block header repository
     */
    ChangesTrieBuilderImpl(
        std::shared_ptr<const storage::trie::TrieStorage> storage,
        std::shared_ptr<storage::trie::PolkadotTrieFactory> changes_storage_factory,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo,
        std::shared_ptr<storage::trie::Codec> codec);

    outcome::result<OptChangesTrieConfig> getConfig() const override;

    outcome::result<std::reference_wrapper<ChangesTrieBuilder>> startNewTrie(
        const common::Hash256 &parent) override;

    outcome::result<void> insertExtrinsicsChange(
        const common::Buffer &key,
        const std::vector<primitives::ExtrinsicIndex> &changers) override;

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

    primitives::BlockHash parent_hash_;
    primitives::BlockNumber parent_number_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_header_repo_;
    std::shared_ptr<storage::trie::PolkadotTrieFactory> changes_storage_factory_;
    std::unique_ptr<storage::trie::PolkadotTrie> changes_storage_;
    std::shared_ptr<storage::trie::Codec> codec_;
    common::Logger logger_;
  };

}  // namespace kagome::storage::changes_trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::changes_trie,
                          ChangesTrieBuilderImpl::Error);

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_IMPL_HPP

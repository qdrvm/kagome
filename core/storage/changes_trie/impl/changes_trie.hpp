/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE
#define KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE

#include <boost/variant.hpp>
#include <map>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"

namespace kagome::storage::changes_trie {

  class ChangesTrie {
   public:
    using ExtrinsicsChanges =
        std::map<common::Buffer, std::vector<primitives::ExtrinsicIndex>>;

    static outcome::result<std::unique_ptr<ChangesTrie>> buildFromChanges(
        const primitives::BlockNumber &parent_block,
        const std::shared_ptr<storage::trie::PolkadotTrieFactory>& trie_factory,
        std::shared_ptr<trie::Codec> codec,
        const ExtrinsicsChanges &extinsics_changes,
        const ChangesTrieConfig &config);

    common::Hash256 getHash() const;

   private:
    ChangesTrie(std::unique_ptr<trie::PolkadotTrie> trie,
                std::shared_ptr<trie::Codec> codec);

    /**
     * key of a changes trie
     */
    struct KeyIndex {
      // block in which the change occurred
      primitives::BlockNumber block {};
      // changed key
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

    std::unique_ptr<trie::PolkadotTrie> changes_trie_;
    std::shared_ptr<trie::Codec> codec_;
    common::Logger logger_;
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE

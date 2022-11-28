/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/common.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/visitor.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"

namespace kagome::blockchain {

  outcome::result<std::optional<common::Buffer>> idToLookupKey(
      const ReadableBufferStorage &map, const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [](const primitives::BlockNumber &n) {
          return prependPrefix(numberToIndexKey(n),
                               prefix::Prefix::ID_TO_LOOKUP_KEY);
        },
        [](const common::Hash256 &hash) {
          return prependPrefix(hash, prefix::Prefix::ID_TO_LOOKUP_KEY);
        });

    OUTCOME_TRY(key_opt, map.tryLoad(key));

    return std::move(key_opt);
  }

  storage::trie::RootHash trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals) {
    auto trie = storage::trie::PolkadotTrieImpl();
    auto codec = storage::trie::PolkadotCodec();

    for (const auto &[key, val] : key_vals) {
      [[maybe_unused]] auto res = trie.put(key, val);
      BOOST_ASSERT_MSG(res.has_value(), "Insertion into trie failed");
    }
    auto root = trie.getRoot();
    if (root == nullptr) {
      static const auto zero_hash = codec.hash256(common::Buffer{0});
      return zero_hash;
    }
    auto encode_res = codec.encodeNode(
        *root, storage::trie::StateVersion::TODO_NotSpecified, {});
    BOOST_ASSERT_MSG(encode_res.has_value(), "Trie encoding failed");
    return codec.hash256(encode_res.value());
  }
}  // namespace kagome::blockchain

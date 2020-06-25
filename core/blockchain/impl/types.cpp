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

OUTCOME_CPP_DEFINE_CATEGORY(kagome::blockchain, Error, e) {
  switch (e) {
    case kagome::blockchain::Error::BLOCK_NOT_FOUND:
      return "Block with such ID is not found";
  }
  return "Unknown error";
}

namespace kagome::blockchain {

  outcome::result<common::Buffer> idToLookupKey(const ReadableBufferMap &map,
                                                const primitives::BlockId &id) {
    auto key = visit_in_place(
        id,
        [&map](const primitives::BlockNumber &n) {
          auto key = prependPrefix(numberToIndexKey(n),
                                   prefix::Prefix::ID_TO_LOOKUP_KEY);
          return map.get(key);
        },
        [&map](const common::Hash256 &hash) {
          return map.get(prependPrefix(common::Buffer{hash},
                                       prefix::Prefix::ID_TO_LOOKUP_KEY));
        });
    if (!key && isNotFoundError(key.error())) {
      return Error::BLOCK_NOT_FOUND;
    }
    return key;
  }

  common::Buffer trieRoot(
      const std::vector<std::pair<common::Buffer, common::Buffer>> &key_vals) {
    auto trie = storage::trie::PolkadotTrieImpl();
    auto codec = storage::trie::PolkadotCodec();

    for (const auto &[key, val] : key_vals) {
      auto res = trie.put(key, val);
      BOOST_ASSERT_MSG(res.has_value(), "Insertion into trie failed");
    }
    auto root = trie.getRoot();
    if (root == nullptr) {
      return common::Buffer{codec.hash256({0})};
    }
    auto encode_res = codec.encodeNode(*root);
    BOOST_ASSERT_MSG(encode_res.has_value(), "Trie encoding failed");
    return common::Buffer{codec.hash256(encode_res.value())};
  }
}  // namespace kagome::blockchain

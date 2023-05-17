/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CODEC_MOCK_HPP
#define KAGOME_CODEC_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/serialization/codec.hpp"

namespace kagome::storage::trie {

  class CodecMock : public trie::Codec {
   public:
    MOCK_METHOD(outcome::result<Buffer>,
                encodeNode,
                (const trie::TrieNode &opaque_node,
                 std::optional<trie::StateVersion> version,
                 const ChildVisitor &child_visitor),
                (const, override));
    MOCK_METHOD(outcome::result<std::shared_ptr<trie::TrieNode>>,
                decodeNode,
                (gsl::span<const uint8_t> encoded_data),
                (const, override));
    MOCK_METHOD(Buffer,
                merkleValue,
                (const BufferView &buf),
                (const, override));
    MOCK_METHOD(outcome::result<Buffer>,
                merkleValue,
                (const trie::OpaqueTrieNode &opaque_node,
                 std::optional<trie::StateVersion> version,
                 const ChildVisitor &child_visitor),
                (const, override));
    MOCK_METHOD(bool, isMerkleHash, (const BufferView &buf), (const, override));
    MOCK_METHOD(common::Hash256,
                hash256,
                (const BufferView &buf),
                (const, override));
    MOCK_METHOD(bool,
                shouldBeHashed,
                (const trie::ValueAndHash &value,
                 std::optional<trie::StateVersion> version_opt),
                (const, override));
  };
}  // namespace kagome::storage::trie
#endif  // KAGOME_CODEC_MOCK_HPP

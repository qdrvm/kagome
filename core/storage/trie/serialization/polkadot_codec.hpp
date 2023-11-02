/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <optional>
#include <string>

#include "codec.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"
#include "storage/trie/serialization/buffer_stream.hpp"

namespace kagome::storage::trie {

  class PolkadotCodec : public Codec {
   public:
    using Buffer = common::Buffer;
    using BufferView = common::BufferView;

    enum class Error {
      SUCCESS = 0,
      TOO_MANY_NIBBLES,   ///< number of nibbles in key is >= 2**16
      UNKNOWN_NODE_TYPE,  ///< node type is unknown
      INPUT_TOO_SMALL,    ///< cannot decode a node, not enough bytes on input
      NO_NODE_VALUE       ///< leaf node without value
    };

    ~PolkadotCodec() override = default;

    outcome::result<Buffer> encodeNode(
        const TrieNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const override;

    outcome::result<std::shared_ptr<TrieNode>> decodeNode(
        BufferView encoded_data) const override;

    MerkleValue merkleValue(const BufferView &buf) const override;
    outcome::result<MerkleValue> merkleValue(
        const OpaqueTrieNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const override;

    common::Hash256 hash256(const BufferView &buf) const override;

    /**
     * Encodes a node header according to the specification
     * @see Algorithm 3: partial key length encoding
     */
    outcome::result<Buffer> encodeHeader(const TrieNode &node,
                                         StateVersion version) const;

   private:
    outcome::result<void> encodeValue(
        common::Buffer &out,
        const TrieNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const;

    outcome::result<Buffer> encodeBranch(
        const BranchNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor) const;
    outcome::result<Buffer> encodeLeaf(const LeafNode &node,
                                       StateVersion version,
                                       const ChildVisitor &child_visitor) const;

    outcome::result<std::pair<TrieNode::Type, size_t>> decodeHeader(
        BufferStream &stream) const;

    outcome::result<KeyNibbles> decodePartialKey(size_t nibbles_num,
                                                 BufferStream &stream) const;

    outcome::result<std::shared_ptr<TrieNode>> decodeBranch(
        TrieNode::Type type,
        const KeyNibbles &partial_key,
        BufferStream &stream) const;

    bool shouldBeHashed(const ValueAndHash &value,
                        StateVersion version) const override;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotCodec::Error);

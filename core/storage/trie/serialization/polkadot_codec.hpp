/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_POLKADOT_CODEC_IMPL_HPP
#define KAGOME_TRIE_POLKADOT_CODEC_IMPL_HPP

#include <memory>
#include <optional>
#include <string>

#include "storage/trie/serialization/buffer_stream.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

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

    outcome::result<Buffer> encodeNode(const Node &node) const override;

    outcome::result<std::shared_ptr<Node>> decodeNode(
        gsl::span<const uint8_t> encoded_data) const override;

    common::Buffer merkleValue(const BufferView &buf) const override;

    common::Hash256 hash256(const BufferView &buf) const override;

    /**
     * Encodes a node header according to the specification
     * @see Algorithm 3: partial key length encoding
     */
    outcome::result<Buffer> encodeHeader(const TrieNode &node) const;

   private:
    outcome::result<Buffer> encodeBranch(const BranchNode &node) const;
    outcome::result<Buffer> encodeLeaf(const LeafNode &node) const;

    outcome::result<std::pair<TrieNode::Type, size_t>> decodeHeader(
        BufferStream &stream) const;

    outcome::result<KeyNibbles> decodePartialKey(size_t nibbles_num,
                                             BufferStream &stream) const;

    outcome::result<std::shared_ptr<Node>> decodeBranch(
        TrieNode::Type type,
        const KeyNibbles &partial_key,
        BufferStream &stream) const;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotCodec::Error);

#endif  // KAGOME_TRIE_POLKADOT_CODEC_IMPL_HPP

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
#include "storage/trie/polkadot_trie/polkadot_node.hpp"

namespace kagome::storage::trie {

  class PolkadotCodec : public Codec {
   public:
    using Buffer = kagome::common::Buffer;

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
        const common::Buffer &encoded_data) const override;

    common::Buffer merkleValue(const Buffer &buf) const override;

    common::Hash256 hash256(const Buffer &buf) const override;

    /**
     * Def. 14 KeyEncode
     * Splits a key to an array of nibbles (a nibble is a half of a byte)
     */
    static Buffer keyToNibbles(const Buffer &key);

    /**
     * Collects an array of nibbles to a key
     */
    static Buffer nibblesToKey(const Buffer &nibbles);

    /**
     * Encodes a node header accroding to the specification
     * @see Algorithm 3: partial key length encoding
     */
    outcome::result<Buffer> encodeHeader(const PolkadotNode &node) const;

   private:
    outcome::result<Buffer> encodeBranch(const BranchNode &node) const;
    outcome::result<Buffer> encodeLeaf(const LeafNode &node) const;

    outcome::result<std::pair<PolkadotNode::Type, size_t>> decodeHeader(
        BufferStream &stream) const;

    outcome::result<Buffer> decodePartialKey(size_t nibbles_num,
                                             BufferStream &stream) const;

    outcome::result<std::shared_ptr<Node>> decodeBranch(
        PolkadotNode::Type type,
        const Buffer &partial_key,
        BufferStream &stream) const;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, PolkadotCodec::Error);

#endif  // KAGOME_TRIE_POLKADOT_CODEC_IMPL_HPP

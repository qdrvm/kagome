/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_UTIL_IMPL_HPP
#define KAGOME_MERKLE_UTIL_IMPL_HPP

#include <memory>
#include <string>

#include "common/byte_stream.hpp"
#include "storage/merkle/codec.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"

namespace kagome::storage::merkle {

  class PolkadotCodec : public Codec {
   public:
    using Buffer = kagome::common::Buffer;

    enum class Error {
      SUCCESS = 0,
      TOO_MANY_NIBBLES,   ///< number of nibbles in key is >= 2**16
      UNKNOWN_NODE_TYPE,  ///< node type is unknown
      INPUT_TOO_SMALL     ///< cannot decode a node, not enough bytes on input
    };

    ~PolkadotCodec() override = default;

    outcome::result<Buffer> encodeNode(const Node &node) const override;
    outcome::result<std::shared_ptr<Node>> decodeNode(
        common::ByteStream &stream) const override;

    common::Hash256 hash256(const Buffer &buf) const override;

    /// non-overriding helper methods

    // definition 14 KeyEncode
    static Buffer keyToNibbles(const Buffer &key);

    // 7.2 Hex encoding
    static Buffer nibblesToKey(const Buffer &key);

    // Algorithm 3: partial key length encoding
    outcome::result<Buffer> getHeader(const PolkadotNode &node) const;

   private:
    outcome::result<Buffer> encodeBranch(const BranchNode &node) const;
    outcome::result<Buffer> encodeLeaf(const LeafNode &node) const;
  };

}  // namespace kagome::storage::merkle

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::merkle, PolkadotCodec::Error);

#endif  // KAGOME_MERKLE_UTIL_IMPL_HPP

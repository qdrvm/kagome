/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_UTIL_IMPL_HPP
#define KAGOME_MERKLE_UTIL_IMPL_HPP

#include <memory>
#include <string>

#include "scale/scale_codec.hpp"
#include "storage/merkle/codec.hpp"
#include "storage/merkle/polkadot_trie_db/node.hpp"

namespace kagome::storage::merkle {

  class PolkadotCodec : public Codec {
   public:
    using Buffer = kagome::common::Buffer;
    using ScaleBufferEncoder = scale::ScaleEncoder<Buffer>;

    enum class Error {
      SUCCESS = 0,
      TOO_MANY_NIBBLES = 1,  ///< number of nibbles in key is >= 2**16
      UNKNOWN_NODE_TYPE = 2  ///< node type is unknown
    };

    ~PolkadotCodec() override = default;

    explicit PolkadotCodec(std::shared_ptr<ScaleBufferEncoder> codec);

    outcome::result<Buffer> nodeEncode(const Node &node) const override;

    common::Buffer hash256(Buffer buf) const override;

    /// non-overriding helper methods

    // definition 14 KeyEncode
    Buffer keyToNibbles(const Buffer &key) const;

    // 7.2 Hex encoding
    Buffer nibblesToKey(const Buffer &key) const;

    // Algorithm 3: partial key length encoding
    outcome::result<Buffer> getHeader(const PolkadotNode &node) const;

   private:
    outcome::result<Buffer> encodeBranch(const BranchNode &node) const;
    outcome::result<Buffer> encodeLeaf(const LeafNode &node) const;

    std::shared_ptr<ScaleBufferEncoder> scale_;
  };

}  // namespace kagome::storage::merkle

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::merkle, PolkadotCodec::Error);

#endif  // KAGOME_MERKLE_UTIL_IMPL_HPP

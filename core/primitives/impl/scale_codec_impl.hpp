/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIMITIVES_SCALE_CODEC_IMPL_HPP
#define KAGOME_PRIMITIVES_SCALE_CODEC_IMPL_HPP

#include "primitives/scale_codec.hpp"
#include "scale/types.hpp"

namespace kagome::primitives {
  class ScaleCodecImpl : public ScaleCodec {
   public:
    ~ScaleCodecImpl() override = default;

    outcome::result<Buffer> encodeBlock(const Block &block) const override;

    outcome::result<Block> decodeBlock(Stream &stream) const override;

    outcome::result<Buffer> encodeBlockHeader(
        const BlockHeader &block_header) const override;

    outcome::result<BlockHeader> decodeBlockHeader(
        Stream &stream) const override;

    outcome::result<Buffer> encodeExtrinsic(
        const Extrinsic &extrinsic) const override;

    outcome::result<Extrinsic> decodeExtrinsic(Stream &stream) const override;

    outcome::result<Buffer> encodeVersion(
        const Version &version) const override;

    outcome::result<Version> decodeVersion(Stream &stream) const override;

    outcome::result<Buffer> encodeBlockId(
        const BlockId &blockId) const override;

    outcome::result<BlockId> decodeBlockId(Stream &stream) const override;

    outcome::result<Buffer> encodeTransactionValidity(
        const TransactionValidity &transactionValidity) const override;

    outcome::result<TransactionValidity> decodeTransactionValidity(
        Stream &stream) const override;

    outcome::result<Buffer> encodeAuthorityIds(
        const std::vector<kagome::primitives::AuthorityId> &ids) const override;

    outcome::result<std::vector<AuthorityId>> decodeAuthorityIds(
        kagome::primitives::ScaleCodec::Stream &stream) const override;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_SCALE_CODEC_IMPL_HPP

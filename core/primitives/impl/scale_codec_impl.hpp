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
    /**
     * @brief virtual destructor
     */
    ~ScaleCodecImpl() override = default;

    /**
     * @brief scale-encodes Block instance
     * @param block value which should be encoded
     * @return scale-encoded value
     */
    outcome::result<Buffer> encodeBlock(const Block &block) const override;

    /**
     * @brief decodes scale-encoded block from stream
     * @param stream source stream containing encoded bytes
     * @return decoded block or error
     */
    outcome::result<Block> decodeBlock(Stream &stream) const override;

    /**
     * @brief scale-encodes BlockHeader instance
     * @param block_header value which should be encoded
     * @return scale-encoded value
     */
    outcome::result<Buffer> encodeBlockHeader(
        const BlockHeader &block_header) const override;

    /**
     * @brief decodes scale-encoded block header from stream
     * @param stream source stream containing encoded bytes
     * @return decoded block header or error
     */
    outcome::result<BlockHeader> decodeBlockHeader(
        Stream &stream) const override;

    /**
     * @brief scale-encodes Extrinsic instance
     * @param extrinsic extrinsic value which should be encoded
     * @return scale-encoded value
     */
    outcome::result<Buffer> encodeExtrinsic(
        const Extrinsic &extrinsic) const override;

    /**
     * @brief decodes scale-encoded extrinsic from stream
     * @param stream source stream containing encoded bytes
     * @return decoded extrinsic or error
     */
    outcome::result<Extrinsic> decodeExtrinsic(Stream &stream) const override;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_PRIMITIVES_SCALE_CODEC_IMPL_HPP

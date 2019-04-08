/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_CODEC_HPP
#define KAGOME_SCALE_CODEC_HPP

#include "common/buffer.hpp"
#include "scale/types.hpp"

#include <outcome/outcome.hpp>

namespace kagome::primitives {
  class Block;  ///< forward declarations of class Block

  class BlockHeader;  ///< forward declarations of class BlockHeader

  class Extrinsic;  ///< forward declarations of class Extrinsic

  class Version;  ///< forward declarations of class Version

  /**
   * class ScaleCodec is an interface declaring methods
   * for encoding and decoding primitives
   */
  class ScaleCodec {
   protected:
    using Stream = common::ByteStream;
    using Buffer = common::Buffer;

   public:
    /**
     * @brief virtual destuctor
     */
    virtual ~ScaleCodec() = default;

    /**
     * @brief scale-encodes Block instance
     * @param block value which should be encoded
     * @return scale-encoded value
     */
    virtual outcome::result<Buffer> encodeBlock(const Block &block) const = 0;

    /**
     * @brief decodes scale-encoded block from stream
     * @param stream source stream containing encoded bytes
     * @return decoded block or error
     */
    virtual outcome::result<Block> decodeBlock(Stream &stream) const = 0;

    /**
     * @brief scale-encodes BlockHeader instance
     * @param block_header value which should be encoded
     * @return scale-encoded value
     */
    virtual outcome::result<Buffer> encodeBlockHeader(
        const BlockHeader &block_header) const = 0;

    /**
     * @brief decodes scale-encoded block header from stream
     * @param stream source stream containing encoded bytes
     * @return decoded block header or error
     */
    virtual outcome::result<BlockHeader> decodeBlockHeader(
        Stream &stream) const = 0;

    /**
     * @brief scale-encodes Extrinsic instance
     * @param extrinsic extrinsic value which should be encoded
     * @return scale-encoded value
     */
    virtual outcome::result<Buffer> encodeExtrinsic(
        const Extrinsic &extrinsic) const = 0;

    /**
     * @brief decodes scale-encoded extrinsic from stream
     * @param stream source stream containing encoded bytes
     * @return decoded extrinsic or error
     */
    virtual outcome::result<Extrinsic> decodeExtrinsic(
        Stream &stream) const = 0;

    virtual outcome::result<Version> decodeVersion(Stream &stream) const = 0;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_SCALE_CODEC_HPP

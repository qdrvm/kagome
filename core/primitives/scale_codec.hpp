/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_CODEC_HPP
#define KAGOME_SCALE_CODEC_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/byte_stream.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/transaction_validity.hpp"
#include "scale/types.hpp"

namespace kagome::primitives {
  class Block;  ///< forward declaration of class Block

  class BlockHeader;  ///< forward declaration of class BlockHeader

  class Extrinsic;  ///< forward declaration of class Extrinsic

  class Version;  ///< forward declaration of class Version

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
     * @return scale-encoded value or error
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
     * @return scale-encoded value or error
     */
    virtual outcome::result<Buffer> encodeBlockHeader(
        const BlockHeader &block_header) const = 0;

    /**
     * @brief decodes scale-encoded block header
     * @param stream source stream containing encoded bytes
     * @return decoded block header or error
     */
    virtual outcome::result<BlockHeader> decodeBlockHeader(
        Stream &stream) const = 0;

    /**
     * @brief scale-encodes Extrinsic instance
     * @param extrinsic value which should be encoded
     * @return scale-encoded value or error
     */
    virtual outcome::result<Buffer> encodeExtrinsic(
        const Extrinsic &extrinsic) const = 0;

    /**
     * @brief decodes scale-encoded extrinsic
     * @param stream source stream containing encoded bytes
     * @return decoded extrinsic or error
     */
    virtual outcome::result<Extrinsic> decodeExtrinsic(
        Stream &stream) const = 0;

    /**
     * @brief scale-encodes Version instance
     * @param version value which should be encoded
     * @return scale-encoded value or error
     */
    virtual outcome::result<Buffer> encodeVersion(
        const Version &version) const = 0;

    /**
     * @brief decodes scale-encoded Version instance
     * @param stream source stream containing encoded bytes
     * @return decoded Version instance or error
     */
    virtual outcome::result<Version> decodeVersion(Stream &stream) const = 0;

    /**
     * @brief scale-encodes block id
     * @param blockId value which should be encoded
     * @return scale-encoded value or error
     */
    virtual outcome::result<Buffer> encodeBlockId(
        const BlockId &blockId) const = 0;

    /**
     * @brief decodes scale-encoded BlockId instance
     * @param stream source stream containing encoded bytes
     * @return decoded BlockId instance or error
     */
    virtual outcome::result<BlockId> decodeBlockId(Stream &stream) const = 0;

    /**
     * @brief scale-encodes TransactionValidity instance
     * @param transactionValidity value which should be encoded
     * @return encoded value or error
     */
    virtual outcome::result<Buffer> encodeTransactionValidity(
        const TransactionValidity &transactionValidity) const = 0;

    /**
     * @brief decodes scale-encoded TransactionValidity instance
     * @param stream source stream containing encoded bytes
     * @return decoded TransactionValidity instance or error
     */
    virtual outcome::result<TransactionValidity> decodeTransactionValidity(
        Stream &stream) const = 0;

    /**
     * @brief scale-encodes AuthorityIds instance
     * @param ids value which should be encoded
     * @return encoded value or error
     */
    virtual outcome::result<Buffer> encodeAuthorityIds(
        const std::vector<AuthorityId> &ids) const = 0;

    /**
     * @brief decodes scale-encoded
     * @param stream source stream containing encoded bytes
     * @return decoded Vector of AuthorityIds or error
     */
    virtual outcome::result<std::vector<AuthorityId>> decodeAuthorityIds(
        Stream &stream) const = 0;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_SCALE_CODEC_HPP

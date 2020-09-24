/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_CHAIN_API_HPP
#define KAGOME_API_CHAIN_API_HPP

#include <boost/variant.hpp>
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::api {
  /**
   * @class ChainApi privides interface for blockchain api
   */
  class ChainApi {
   public:
    virtual ~ChainApi() = default;
    using BlockNumber = primitives::BlockNumber;
    using BlockHash = kagome::primitives::BlockHash;
    using ValueType = boost::variant<BlockNumber, std::string>;

    /**
     * @return last finalized block hash
     */
    virtual outcome::result<BlockHash> getBlockHash() const = 0;

    /**
     * @param block_number block number
     * @return block hash by number
     */
    virtual outcome::result<BlockHash> getBlockHash(
        BlockNumber block_number) const = 0;

    /**
     * @param hex_number hex-encoded block number
     * @return block hash by number
     */
    virtual outcome::result<BlockHash> getBlockHash(
        std::string_view hex_number) const = 0;

    /**
     * @param values mixed values array either of block number of hex-encoded
     * block number as string
     * @return array of block hashes for numbers
     */
    virtual outcome::result<std::vector<BlockHash>> getBlockHash(
        gsl::span<const ValueType> values) const = 0;

    /**
     * @param hash hex-string of a block to retrieve
     * @return BlockHeader data structure
     */
    virtual outcome::result<primitives::BlockHeader> getHeader(
        std::string_view hash) = 0;

    /**
     * Returns header of a last finalized block.
     * @return BlockHeader data structure
     */
    virtual outcome::result<primitives::BlockHeader> getHeader() = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_CHAIN_API_HPP

/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_CHAIN_API_HPP
#define KAGOME_API_CHAIN_API_HPP

#include <boost/variant.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"

namespace kagome::api {
  class ApiService;

  /**
   * @class ChainApi provides interface for blockchain api
   */
  class ChainApi {
   public:
    virtual ~ChainApi() = default;
    using BlockNumber = primitives::BlockNumber;
    using BlockHash = kagome::primitives::BlockHash;
    using ValueType = boost::variant<BlockNumber, std::string>;

    virtual void setApiService(
        const std::shared_ptr<api::ApiService> &api_service) = 0;

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
     */
    virtual outcome::result<primitives::BlockHeader> getHeader(
        std::string_view hash) = 0;

    /**
     * Returns header of a last finalized block.
     */
    virtual outcome::result<primitives::BlockHeader> getHeader() = 0;

    /**
     * @param hash hex-string of a block to retrieve
     */
    virtual outcome::result<primitives::BlockData> getBlock(
        std::string_view hash) = 0;

    /**
     * Returns header of a last finalized block.
     */
    virtual outcome::result<primitives::BlockData> getBlock() = 0;

    /**
     * Get hash of the last finalized block in the canon chain.
     * @returns The hash of the last finalized block
     */
    virtual outcome::result<primitives::BlockHash> getFinalizedHead() const = 0;

    /**
     * Subscribes to events of Finalized Heads type.
     * @return id of the subscription
     */
    virtual outcome::result<uint32_t> subscribeFinalizedHeads() = 0;

    /**
     * Unsubscribes from events of Finalized Heads type.
     */
    virtual outcome::result<void> unsubscribeFinalizedHeads(
        uint32_t subscription_id) = 0;

    /**
     * Subscribes to events of New Heads type
     * @return id of the subscription
     */
    virtual outcome::result<uint32_t> subscribeNewHeads() = 0;

    /**
     * Unsubscribes from events of New Heads type.
     */
    virtual outcome::result<void> unsubscribeNewHeads(
        uint32_t subscription_id) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_CHAIN_API_HPP

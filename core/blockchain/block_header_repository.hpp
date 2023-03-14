/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP
#define KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP

#include <optional>

#include "common/blob.hpp"
#include "common/visitor.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"

namespace kagome::blockchain {

  /**
   * Status of a block
   */
  enum class BlockStatus { InChain, Unknown };

  /**
   * An interface to a storage with block headers that provides several
   * convenience methods, such as getting bloch number by its hash and vice
   * versa or getting a block status
   */
  class BlockHeaderRepository {
   public:
    virtual ~BlockHeaderRepository() = default;

    /**
     * @param hash - a blake2_256 hash of an SCALE encoded block header
     * @return the number of the block with the provided hash in case one is in
     * the storage or an error
     */
    virtual outcome::result<primitives::BlockNumber> getNumberByHash(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * @param number - the number of a block, contained in a block header
     * @return the hash of the block with the provided number in case one is in
     * the storage or an error
     */
    virtual outcome::result<primitives::BlockHash> getHashByNumber(
        primitives::BlockNumber block_number) const = 0;

    /**
     * @return block header with corresponding id or an error
     */
    virtual outcome::result<primitives::BlockHeader> getBlockHeader(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * @param id of a block which status is returned
     * @return status of a block or a storage error
     */
    virtual outcome::result<BlockStatus> getBlockStatus(
        const primitives::BlockHash &block_hash) const = 0;

    /**
     * @param id of a block which number is returned
     * @return block number or a none optional if the corresponding block header
     * is not in storage or a storage error
     */
    outcome::result<primitives::BlockNumber> getNumberById(
        const primitives::BlockId &block_id) const {
      return visit_in_place(
          block_id,
          [](const primitives::BlockNumber &block_number) {
            return block_number;
          },
          [this](const primitives::BlockHash &block_hash) {
            return getNumberByHash(block_hash);
          });
    }

    /**
     * @param id of a block which hash is returned
     * @return block hash or a none optional if the corresponding block header
     * is not in storage or a storage error
     */
    outcome::result<primitives::BlockHash> getHashById(
        const primitives::BlockId &id) const {
      return visit_in_place(
          id,
          [this](const primitives::BlockNumber &n) {
            return getHashByNumber(n);
          },
          [](const primitives::BlockHash &hash) { return hash; });
    }
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_CORE_BLOCKCHAIN_BLOCK_HEADER_REPOSITORY_HPP

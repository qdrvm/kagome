/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/block_builder_factory.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"

namespace kagome::authorship {

  /**
   * @class BlockBuilderFactoryImpl
   * @brief The BlockBuilderFactoryImpl class is responsible for creating
   * instances of BlockBuilder.
   *
   * @see BlockBuilderFactory
   * @see BlockBuilder
   */
  class BlockBuilderFactoryImpl : public BlockBuilderFactory {
   public:
    ~BlockBuilderFactoryImpl() override = default;

    /**
     * @brief Constructs a new BlockBuilderFactoryImpl.
     *
     * @param r_core Shared pointer to the runtime core runtime API that is used
     * to initialize the block.
     * @param r_block_builder Shared pointer to the runtime block builder
     * runtime API that is used to build the block.
     * @param header_backend Shared pointer to the block header repository to be
     * used to get the block number by its hash.
     */
    BlockBuilderFactoryImpl(
        std::shared_ptr<runtime::Core> r_core,
        std::shared_ptr<runtime::BlockBuilder> r_block_builder,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_backend);

    /**
     * @brief Creates a new BlockBuilder instance.
     *
     * @param parent The parent block info.
     * @param inherent_digest The inherent digest to be used to build the new
     * block.
     * @param changes_tracker The changes tracker to be used to track trie
     * changes.
     * @return A result containing a unique pointer to the new BlockBuilder
     * instance, or an error if the operation failed.
     */
    outcome::result<std::unique_ptr<BlockBuilder>> make(
        const kagome::primitives::BlockInfo &parent_block,
        primitives::Digest inherent_digest,
        TrieChangesTrackerOpt changes_tracker) const override;

   private:
    std::shared_ptr<runtime::Core> r_core_;
    std::shared_ptr<runtime::BlockBuilder> r_block_builder_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_backend_;
    log::Logger logger_;
  };

}  // namespace kagome::authorship

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/block_builder.hpp"

#include "log/logger.hpp"
#include "primitives/block_id.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/block_builder.hpp"
#include "runtime/runtime_api/core.hpp"
#include "runtime/runtime_context.hpp"

namespace kagome::authorship {

  /**
   * @class BlockBuilderImpl
   * @file core/authorship/impl/block_builder_impl.hpp
   * @brief The BlockBuilderImpl class is responsible for building blocks.
   *
   * This class implements the BlockBuilder interface. It uses the provided
   * block header and other parameters to construct a new block.
   *
   * @see BlockBuilder
   */
  class BlockBuilderImpl : public BlockBuilder {
   public:
    ~BlockBuilderImpl() override = default;

    /**
     * @brief Constructs a new BlockBuilderImpl.
     * @file core/authorship/impl/block_builder_impl.cpp
     *
     * @param header The block header to be used to build the block.
     * @param context The runtime context containing the runtime parameters such
     * as memory limits.
     * @param block_builder Shared pointer to the block builder runtime API
     */
    BlockBuilderImpl(primitives::BlockHeader block_header,
                     std::unique_ptr<runtime::RuntimeContext> ctx,
                     std::shared_ptr<runtime::BlockBuilder> block_builder_api);

    /**
     * Retrieves the inherent extrinsics for the block from provided inherent
     * data.
     *
     * @param data The inherent data to be used.
     * @return A vector of inherent extrinsics.
     */
    outcome::result<std::vector<primitives::Extrinsic>> getInherentExtrinsics(
        const primitives::InherentData &data) const override;

    /**
     * Pushes an extrinsic into the block.
     *
     * @param extrinsic The extrinsic to be pushed.
     * @return The index of the extrinsic in the block.
     */
    outcome::result<primitives::ExtrinsicIndex> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    /**
     * Finalizes the block construction and returns the built block.
     *
     * @return The built block.
     */
    outcome::result<primitives::Block> bake() const override;

    /**
     * Estimates the size of the block based on
     *
     * This function calculates the size of the block by encoding it
     * using the ScaleEncoderStream
     *
     * @return The estimated size of the block.
     */
    size_t estimateBlockSize() const override;

   private:
    /**
     * @brief Returns the estimated size of the block header.
     *
     * This function calculates the size of the block header by encoding it
     * using the ScaleEncoderStream
     *
     * @return The estimated size of the block header.
     */
    size_t estimatedBlockHeaderSize() const;

    primitives::BlockHeader block_header_;
    std::shared_ptr<runtime::BlockBuilder> block_builder_api_;
    std::unique_ptr<runtime::RuntimeContext> ctx_;
    log::Logger logger_;

    std::vector<primitives::Extrinsic> extrinsics_{};
  };

}  // namespace kagome::authorship

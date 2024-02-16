/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::authorship {

  /**
   * The BlockBuilder class is responsible for collecting extrinsics, creating a
   * new block, and should be destroyed after the block is built. It provides
   * methods for getting inherent extrinsics, adding extrinsics to a block,
   * finalizing the construction of a block, and estimating the size of the
   * encoded block. This class is used in the block production process,
   * specifically in the propose method of the ProposerImpl class.
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * The getInherentExtrinsics method retrieves the inherent extrinsics based
     * on the provided inherent data. This method is called in the propose
     * method of the ProposerImpl class.
     *
     * @param data The inherent data used to retrieve the inherent extrinsics.
     * @return A result containing a vector of inherent extrinsics if they were
     * successfully retrieved, or an error if the extrinsics could not be
     * retrieved.
     */
    virtual outcome::result<std::vector<primitives::Extrinsic>>
    getInherentExtrinsics(const primitives::InherentData &data) const = 0;

    /**
     * The pushExtrinsic method adds an extrinsic to the block being built.
     * This method is called in the propose method of the ProposerImpl class.
     *
     * @param extrinsic The extrinsic to be added to the block.
     * @return A result containing the index of the extrinsic if it was
     * successfully added, or an error if the extrinsic could not be added.
     */
    virtual outcome::result<primitives::ExtrinsicIndex> pushExtrinsic(
        const primitives::Extrinsic &extrinsic) = 0;

    /**
     * The bake method finalizes the construction of the block and returns the
     * built block. This method is called in the propose method of the
     * ProposerImpl class after all extrinsics have been added to the block.
     *
     * @return A result containing the built block if the block was successfully
     * built, or an error if the block could not be built.
     */
    virtual outcome::result<primitives::Block> bake() const = 0;

    /**
     * The estimateBlockSize method estimates the size of the encoded block
     * representation. This method can be used to check if the block size limit
     * has been reached.
     *
     * @return The estimated size of the encoded block in bytes.
     */
    virtual size_t estimateBlockSize() const = 0;
  };

}  // namespace kagome::authorship

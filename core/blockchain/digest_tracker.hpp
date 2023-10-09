/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"
#include "primitives/digest.hpp"

namespace kagome::blockchain {

  class DigestTracker {
   public:
    virtual ~DigestTracker() = default;

    /**
     * Processes block digest
     * @param context - data of accorded block
     * @param digest - applying digest
     * @return failure or nothing
     */
    virtual outcome::result<void> onDigest(
        const primitives::BlockContext &context,
        const primitives::Digest &digest) = 0;

    /**
     * Cancels digest of applied block.
     * Should be called when the block is rolling back
     * @param block - corresponding block
     */
    virtual void cancel(const primitives::BlockInfo &block) = 0;
  };

}  // namespace kagome::blockchain

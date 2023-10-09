/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/empty.hpp"
#include "primitives/mmr.hpp"

namespace kagome::runtime {
  class MmrApi {
   public:
    virtual ~MmrApi() = default;

    template <typename T>
    using Result = outcome::result<primitives::MmrResult<T>>;

    virtual Result<common::Hash256> mmrRoot(
        const primitives::BlockHash &block) = 0;

    using GenerateProof =
        std::pair<primitives::MmrLeaves, primitives::MmrProof>;
    virtual Result<GenerateProof> generateProof(
        const primitives::BlockHash &block,
        std::vector<primitives::BlockNumber> block_numbers,
        std::optional<primitives::BlockNumber> best_known_block_number) = 0;

    virtual Result<Empty> verifyProof(const primitives::BlockHash &block,
                                      const primitives::MmrLeaves &leaves,
                                      const primitives::MmrProof &proof) = 0;

    virtual Result<Empty> verifyProofStateless(
        const primitives::BlockHash &block,
        const common::Hash256 &mmr_root,
        const primitives::MmrLeaves &leaves,
        const primitives::MmrProof &proof) = 0;
  };
}  // namespace kagome::runtime

/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/common.hpp"
#include "runtime/runtime_api/mmr.hpp"

namespace kagome::runtime {
  class Executor;

  class MmrApiImpl : public MmrApi {
   public:
    MmrApiImpl(std::shared_ptr<Executor> executor);

    Result<common::Hash256> mmrRoot(
        const primitives::BlockHash &block) override;
    Result<GenerateProof> generateProof(
        const primitives::BlockHash &block,
        std::vector<primitives::BlockNumber> block_numbers,
        std::optional<primitives::BlockNumber> best_known_block_number)
        override;
    Result<Empty> verifyProof(const primitives::BlockHash &block,
                              const primitives::MmrLeaves &leaves,
                              const primitives::MmrProof &proof) override;
    Result<Empty> verifyProofStateless(
        const primitives::BlockHash &block,
        const common::Hash256 &mmr_root,
        const primitives::MmrLeaves &leaves,
        const primitives::MmrProof &proof) override;

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime

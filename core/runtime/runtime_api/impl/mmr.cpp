/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/mmr.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {
  MmrApiImpl::MmrApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  MmrApi::Result<common::Hash256> MmrApiImpl::mmrRoot(
      const primitives::BlockHash &block) {
    return executor_->callAt<primitives::MmrResult<common::Hash256>>(
        block, "MmrApi_mmr_root");
  }

  MmrApi::Result<MmrApi::GenerateProof> MmrApiImpl::generateProof(
      const primitives::BlockHash &block,
      std::vector<primitives::BlockNumber> block_numbers,
      std::optional<primitives::BlockNumber> best_known_block_number) {
    return executor_->callAt<primitives::MmrResult<MmrApi::GenerateProof>>(
        block, "MmrApi_generate_proof", block_numbers, best_known_block_number);
  }

  MmrApi::Result<Empty> MmrApiImpl::verifyProof(
      const primitives::BlockHash &block,
      const primitives::MmrLeaves &leaves,
      const primitives::MmrProof &proof) {
    return executor_->callAt<primitives::MmrResult<Empty>>(
        block, "MmrApi_verify_proof", leaves, proof);
  }

  MmrApi::Result<Empty> MmrApiImpl::verifyProofStateless(
      const primitives::BlockHash &block,
      const common::Hash256 &mmr_root,
      const primitives::MmrLeaves &leaves,
      const primitives::MmrProof &proof) {
    return executor_->callAt<primitives::MmrResult<Empty>>(
        block, "MmrApi_verify_proof_stateless", mmr_root, leaves, proof);
  }
}  // namespace kagome::runtime

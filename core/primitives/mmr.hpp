/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "primitives/common.hpp"

namespace kagome::primitives {
  enum class MmrError : uint8_t {
    InvalidNumericOp,
    Push,
    GetRoot,
    Commit,
    GenerateProof,
    Verify,
    LeafNotFound,
    PalletNotIncluded,
    InvalidLeafIndex,
    InvalidBestKnownBlock,
  };

  template <typename T>
  using MmrResult = boost::variant<T, MmrError>;

  using MmrLeaves = std::vector<common::Buffer>;

  struct MmrProof {
    std::vector<uint64_t> leaf_indices;
    uint64_t leaf_count;
    std::vector<common::Hash256> items;
  };

  struct MmrLeavesProof {
    primitives::BlockHash block_hash;
    common::Buffer leaves;
    common::Buffer proof;
  };
}  // namespace kagome::primitives

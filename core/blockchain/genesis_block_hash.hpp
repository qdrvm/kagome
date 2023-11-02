/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/block_tree.hpp"

namespace kagome::blockchain {

  class BlockTree;

  class GenesisBlockHash final : public primitives::BlockHash {
   public:
    GenesisBlockHash(std::shared_ptr<BlockTree> block_tree);
  };

}  // namespace kagome::blockchain

template <>
struct fmt::formatter<kagome::blockchain::GenesisBlockHash>
    : fmt::formatter<kagome::common::BufferView> {};

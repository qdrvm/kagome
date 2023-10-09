/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/genesis_block_hash.hpp"

#include "blockchain/block_tree.hpp"

namespace kagome::blockchain {

  GenesisBlockHash::GenesisBlockHash(std::shared_ptr<BlockTree> block_tree)
      : primitives::BlockHash(block_tree->getGenesisBlockHash()){};

}  // namespace kagome::blockchain
